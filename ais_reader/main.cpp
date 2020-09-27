
#include "ais_decoder/strutils.h"
#include "ais_decoder/decoder.h"
#include "ais_decoder/processing.h"
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




template <typename payload_type>
using Queue = LockFreeQueue<payload_type, 1024>;

Queue<std::unique_ptr<Fragments>> fragmentQueue;
Queue<std::unique_ptr<Messages>> messageQueue;
Queue<std::unique_ptr<Payloads>> payloadQueue;
String<1024*32, char> nmeaData;


using Clock = std::chrono::high_resolution_clock;
FILE *fin = nullptr;


void readFromFile()
{
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
        int n = fread(nmeaData.data() + nmeaData.size(), 1, nmeaData.maxSize() - nmeaData.size(), fin);
        nmeaData.setSize(nmeaData.size() + n);
    }
    
    size_t bytesUsed = processNmeaData(fragmentQueue, nmeaData);
    if (bytesUsed > 0) {
        // keep dropped data for next read
        size_t droppedSize = nmeaData.size() - bytesUsed;
        memcpy(nmeaData.data(), nmeaData.data() + bytesUsed, droppedSize);
        nmeaData.setSize(droppedSize);
    }
    else {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}


void readMessages() {
    for (;;) {
        readFromFile();
    }
}


void procFragments() {
    for (;;) {
        if (processFragments(messageQueue, fragmentQueue) == 0) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(0));
        }
    }
}


void procMessages() {
    for (;;) {
        if (processMessages(payloadQueue, messageQueue) == 0) {
            //std::this_thread::sleep_for(std::chrono::milliseconds(0));
        }
    }
}


    

int main() {
    new NmeaFrg();
    
    auto ts = Clock::now();
    size_t uMsgCount = 0;
    
    auto thread1 = std::thread(readMessages);
    auto thread2 = std::thread(procFragments);
    auto thread3 = std::thread(procMessages);
    
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
            //std::this_thread::sleep_for(std::chrono::milliseconds(0));
        }
    }
    
    thread1.join();
    thread2.join();
}




