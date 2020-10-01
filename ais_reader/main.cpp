
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
using Queue = BlockingQueue<payload_type, 1024>;

Queue<std::unique_ptr<Fragments>> fragmentQueue;
Queue<std::unique_ptr<Messages>> messageQueue;
Queue<std::unique_ptr<Payloads>> payloadQueue;
String<1024*64> nmeaData;


using Clock = std::chrono::high_resolution_clock;
FILE *fin = nullptr;


void readFromFile()
{
    // read from file
    if (fin == nullptr)
    {
        fin = fopen("data/Smithland.txt", "rb");
    }
    
    if ( (fin != nullptr) &&
         (feof(fin) == 1) )
    {
        fclose(fin);
        fin = nullptr;
        //throw std::runtime_error("file done");
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
}


void readFragments() {
    for (;;) {
        readFromFile();
    }
}


void procFragmentsQueue() {
    for (;;) {
        processFragments(messageQueue, fragmentQueue);
    }
}


void procMessagesQueue() {
    for (;;) {
        processMessages(payloadQueue, messageQueue);
    }
}


void procPayloadsQueue() {
    auto ts = Clock::now();
    size_t uMsgCount = 0;
    
    for (;;) {
        auto pPayloads = payloadQueue.pop();
        if (pPayloads != nullptr) {
            for (MsgPayload &p : *pPayloads) {
                size_t uBitIndex = 0;
                
                int msgType = getUnsignedValue(p, uBitIndex, 6);
                if (msgType == 5)
                {
                    getUnsignedValue(p, uBitIndex, 2);                 // repeatIndicator
                    auto mmsi = getUnsignedValue(p, uBitIndex, 30);
                    getUnsignedValue(p, uBitIndex, 2);                 // AIS version
                    auto imo = getUnsignedValue(p, uBitIndex, 30);
                    auto callsign = getString(p, uBitIndex, 42);
                    auto name = getString(p, uBitIndex, 120);
                    auto type = getUnsignedValue(p, uBitIndex, 8);
                    
                    //printf("mmsi=%lu, callsign=%s, name=%s\n", mmsi, callsign.c_str(), name.c_str());
                }
                else if ( (msgType == 1) ||
                          (msgType == 2) ||
                          (msgType == 3) )
                {
                    getUnsignedValue(p, uBitIndex, 2);                 // repeatIndicator
                    auto mmsi = getUnsignedValue(p, uBitIndex, 30);
                    auto navstatus = getUnsignedValue(p, uBitIndex, 4);
                    auto rot = getSignedValue(p, uBitIndex, 8);
                    auto sog = getUnsignedValue(p, uBitIndex, 10);
                    auto posAccuracy = getBoolValue(p, uBitIndex);
                    auto posLon = getSignedValue(p, uBitIndex, 28);
                    auto posLat = getSignedValue(p, uBitIndex, 27);
                    auto cog = (int)getUnsignedValue(p, uBitIndex, 12);
                    auto heading = (int)getUnsignedValue(p, uBitIndex, 9);
                    
                    getUnsignedValue(p, uBitIndex, 6);     // timestamp
                    getUnsignedValue(p, uBitIndex, 2);     // maneuver indicator
                    getUnsignedValue(p, uBitIndex, 3);     // spare
                    getBoolValue(p, uBitIndex);          // RAIM
                    getUnsignedValue(p, uBitIndex, 19);     // radio status
                    
                    //printf("mmsi=%lu, lon=%lu, lat=%lu\n", mmsi, posLon, posLat);
                }

                uMsgCount++;
                
                if (uMsgCount > 5000000) {
                    auto td = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - ts).count() * 1e-09;
                    printf("messages per second = %.2f\n", (float)(uMsgCount / td));

                    ts = Clock::now();
                    uMsgCount = 0;
                }
            }
        }
    }
}



    

int main() {
    int s1 = sizeof(NmeaFrg);
    int s2 = sizeof(NmeaMsg);
    int s3 = sizeof(MsgPayload);
    
    
    auto thread1 = std::thread(readFragments);
    auto thread2 = std::thread(procFragmentsQueue);
    auto thread3 = std::thread(procMessagesQueue);
    auto thread4 = std::thread(procPayloadsQueue);
    
    for (;;) {
        printf("frgq=%d, msgq=%d, pldq=%d\n",
               (int)fragmentQueue.size(),
               (int)messageQueue.size(),
               (int)payloadQueue.size());
               
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();
}




