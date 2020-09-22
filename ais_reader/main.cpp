
#include "ais_decoder/strutils.h"
#include "ais_decoder/decoder.h"
#include "ais_decoder/ringbuffer.h"

#include <stdlib.h>
#include <stdio.h>
#include <array>
#include <cstring>
#include <chrono>
#include <string>
#include <atomic>
#include <thread>


using Clock = std::chrono::high_resolution_clock;

RingBuffer<NmeaMsg, 4096>       messages;
RingBuffer<MsgPayload, 4096>    payloads;
int                             msgCount = 0;
    

std::array<char, 1024*64>  nmeaData;

void readFromFile() {
    auto *pFile = fopen("data/nmea-sample.txt", "rb");
    if (pFile != nullptr) {
        while (feof(pFile) == 0) {
            // TODO: start with any bytes missed at end of previous buffer
            int n = fread(nmeaData.data(), 1, nmeaData.size(), pFile);
            
            char *pData = nmeaData.data();
            char *pEnd = pData + n;

            while (pData < pEnd) {
                // skip '!' and '$'
                pData = (*pData == '!') ? pData + 1 : pData;
                pData = (*pData == '$') ? pData + 1 : pData;
    
                // NOTE: will keep spinning if message queue is full
                auto pMsg = messages.push();
                if (pMsg != nullptr) {
                    size_t n = readSentence(pMsg, pData, pEnd - pData);
                    if (n > 0) {
                        // read EOLs
                        pData += n;
                        pData = ((pData < pEnd) && (*pData == '\r')) ? pData + 1 : pData;
                        pData = ((pData < pEnd) && (*pData == '\n')) ? pData + 1 : pData;                        
                    }
                    else {
                        // skip to next line
                        pData = (char*)memchr(pData, '\n', pEnd - pData);
                        if (pData == nullptr) {
                            break;
                        }
                        else {
                            pData++;
                        }
                    }
                }
                else {
                    // yield if message queue is full
                    std::this_thread::sleep_for(std::chrono::nanoseconds(0));
                }
            }
                        
        }
        
        fclose(pFile);
    }
}

void readMessages() {
    for (int i = 0; i < 1000; i++) {
        readFromFile();
    }
}


void processMessages() {
    NmeaMsg *pMsg = nullptr;
    for (;;)
    {
        if (pMsg == nullptr) {
            pMsg = messages.pop();
        }
        
        if (pMsg != nullptr)
        {
            auto pPayload = payloads.push();
            if (pPayload != nullptr)
            {
                uint8_t crc = calcCrc(pMsg->m_message);
                if (crc == pMsg->m_uMsgCrc) {
                    int bitsUsed = decodeAscii(pPayload, pMsg->m_payload, pMsg->m_uFillBits);
                    
                    size_t bitIndex = 0;
                    auto msgType = getUnsignedValue(pPayload, bitIndex, 6);
                    
                    msgCount++;
                }
                
                pMsg = nullptr;
            }
            else {
                // yield if payload queue is full
                std::this_thread::sleep_for(std::chrono::nanoseconds(0));
            }
        }
        else {
            // yield if message queue is empty
            std::this_thread::sleep_for(std::chrono::nanoseconds(0));
        }
    }
}


    

int main() {
    auto ts = Clock::now();
    
    auto thread1 = std::thread(readMessages);
    auto thread2 = std::thread(processMessages);
    
    for (;;) {
        if (payloads.pop() == nullptr) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(0));
        }
    }
    
    thread1.join();
    thread2.join();
    
    double td = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - ts).count() * 1e-09;
    printf("messages %lu; per second %.2f\n", (unsigned long)msgCount, (float)(msgCount/td));
}
