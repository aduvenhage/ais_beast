
#include "ais_decoder/strutils.h"
#include "ais_decoder/decoder.h"
#include "ais_decoder/queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <array>
#include <cstring>
#include <chrono>
#include <string>
#include <atomic>
#include <thread>
#include <functional>
#include <queue>
#include <memory>
#include <vector>


template <typename payload_type, int N>
struct Chunk
{
    Chunk()
        :m_count(0),
         m_index(0)
    {}

    payload_type &data() {
        return m_data.data()[m_count];
    }

    void next() {
        m_count++;
    }
    
    void reset() {
        m_count = 0;
        m_index = 0;
    }
    
    bool full() const {
        return m_count >= N;
    }
    
    bool empty() const {
        return m_count == 0;
    }
    
    payload_type *begin() {
        return m_data.data();
    }

    payload_type *end() {
        return m_data.data() + m_count;
    }
    
    size_t count() const {
        return m_count;
    }
    
    size_t maxSize() const {
        return (size_t)N;
    }
    
    std::array<payload_type, N>   m_data;
    size_t                        m_count;      // size used
    size_t                        m_index;      // size processed
};


using NmeaData = Chunk<char, 1024 * 1024>;
using Messages = Chunk<NmeaMsg, 1024 * 8>;
using Payloads = Chunk<MsgPayload, 1024 * 4>;

Queue<std::unique_ptr<NmeaData>, 128> dataQueue;
Queue<std::unique_ptr<Messages>, 1024 * 4> messageQueue;
Queue<std::unique_ptr<Payloads>, 1024 * 4> payloadQueue;


using Clock = std::chrono::high_resolution_clock;
FILE                *fin = nullptr;






void readFromFile() {
    if (messageQueue.full() == false) {
        // create or re-use new data block
        std::unique_ptr<NmeaData> pNmeaData;
        if (dataQueue.full() == true) {
            dataQueue.pop(pNmeaData);
            pNmeaData->reset();
        }
        else {
            pNmeaData = std::make_unique<NmeaData>();
        }
        
        // add dropped data from last data block
        if (dataQueue.empty() == false) {
            auto &pOldNmeaData = dataQueue.back();
            memcpy(pNmeaData->begin(), pOldNmeaData->begin() + pOldNmeaData->m_index, pOldNmeaData->m_count - pOldNmeaData->m_index);
            pNmeaData->m_count = pOldNmeaData->m_count - pOldNmeaData->m_index;
        }
        
        // read from file
        if ( (fin == nullptr) ||
             (feof(fin) == 1) )
        {
            if (fin != nullptr) {
                fclose(fin);
            }
            
            fin = fopen("data/nmea-sample.txt", "rb");
        }
        
        if ( (fin != nullptr) &&
             (feof(fin) == 0) )
        {
            int n = fread(&pNmeaData->data(), 1, pNmeaData->maxSize() - pNmeaData->count(), fin);
            pNmeaData->m_count += n;
        }
        
        // process data
        char *pData = pNmeaData->begin();
        char *pEnd = pNmeaData->end();
        auto pMessages = std::make_unique<Messages>();

        while (pData < pEnd) {
            // skip '!' and '$'
            pData = (*pData == '!') ? pData + 1 : pData;
            pData = (*pData == '$') ? pData + 1 : pData;

            auto &message = pMessages->data();
            size_t n = readSentence(message, pData, pEnd - pData);
            if (n > 0) {
                // read EOLs
                pData += n;
                pData = ((pData < pEnd) && (*pData == '\r')) ? pData + 1 : pData;
                pData = ((pData < pEnd) && (*pData == '\n')) ? pData + 1 : pData;
                pNmeaData->m_index = pData - pNmeaData->begin();
                
                // try to output message
                pMessages->next();
                if (pMessages->full() == true) {
                    messageQueue.push(std::move(pMessages));
                    
                    // stop if queue is full, or continue
                    if (messageQueue.full() == true) {
                        // dropped data should be picked up on next read
                        break;
                    }
                    else {
                        pMessages = std::make_unique<Messages>();
                    }
                }
            }
            else {
                // skip to next line
                auto pNewData = (char*)memchr(pData, '\n', pEnd - pData);
                if (pNewData != nullptr) {
                    pData = pNewData + 1;
                }
                
                // end of data reached
                else {
                    break;
                }
            }
        }

        // output last message
        if (pMessages->empty() == false) {
            messageQueue.push(std::move(pMessages));
        }
        
        // keep data
        dataQueue.push(std::move(pNmeaData));
    }
}


void readMessages() {
    for (;;) {
        readFromFile();
    }
}


void processMessages() {
    for (;;) {
        if ( (messageQueue.empty() == false) &&
             (payloadQueue.full() == false) )
        {
            auto pPayloads = std::make_unique<Payloads>();
            
            std::unique_ptr<Messages> pMessages;
            if (messageQueue.pop(pMessages) == true) {
                auto &messages = *pMessages;
                for (auto &msg : messages) {
                    uint8_t crc = calcCrc(msg.m_message);
                    if (crc == msg.m_uMsgCrc) {
                        auto &payload = pPayloads->data();
                        payload.m_bitsUsed = decodeAscii(payload, msg.m_payload, msg.m_uFillBits);
                        if (payload.m_bitsUsed > 0) {
                            pPayloads->next();
                            if (pPayloads->full() == true) {
                                payloadQueue.push(std::move(pPayloads));
                                pPayloads = std::make_unique<Payloads>();
                            }
                        }
                    }
                }
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(0));
            }

            // output last payloads
            if (pPayloads->empty() == false) {
                payloadQueue.push(std::move(pPayloads));
            }
        }
    }
}


    

int main() {
    auto ts = Clock::now();
    size_t uMsgCount = 0;
    
    auto thread1 = std::thread(readMessages);
    auto thread2 = std::thread(processMessages);
    
    for (;;) {
        std::unique_ptr<Payloads> pPayloads;
        if (payloadQueue.pop(pPayloads) == true) {
            for (MsgPayload &p : *pPayloads) {
                size_t uBitIndex = 0;
                int msgType = getUnsignedValue(p, uBitIndex, 6);
                getUnsignedValue(p, uBitIndex, 2);                 // repeatIndicator
                auto mmsi = getUnsignedValue(p, uBitIndex, 30);
                
                uMsgCount++;
                
                if ( (uMsgCount > 0) && (uMsgCount % 2000000 == 0) ) {
                    auto td = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - ts).count() * 1e-09;
                    printf("message = %lu, per second = %.2f\n", uMsgCount, (float)(uMsgCount / td));
                }
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(0));
        }
    }
    
    thread1.join();
    thread2.join();
}




