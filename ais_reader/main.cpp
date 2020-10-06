
#include "ais_decoder/strutils.h"
#include "ais_decoder/decoder.h"
#include "ais_decoder/processing.h"
#include "ais_decoder/queue.h"
#include "ais_decoder/tiff.h"

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
std::atomic<bool> fileFinished = false;
std::atomic<uint32> msgCount = 0;

const int OUTPUT_WIDTH = 1024 * 4;
const int OUTPUT_HEIGHT = 1024 * 4;
std::vector<uint16_t> image(OUTPUT_WIDTH * OUTPUT_HEIGHT, 0);
int pixelMax = 0;



using Clock = std::chrono::high_resolution_clock;
FILE *fin = nullptr;


bool readFromFile()
{
    if (fin == nullptr)
    {
        fin = fopen("data/Smithland.txt", "rb");
    }
    
    if ( (fin != nullptr) &&
         (feof(fin) == 1) )
    {
        fclose(fin);
        fin = nullptr;
        return false;   // finished reading file
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
    
    return true;    // still busy on file
}


void readFragments() {
    bool hasData = true;
    while (hasData == true) {
        auto bBusy = readFromFile();

        // check if we are done with file
        if (bBusy == false) {
            fileFinished = true;
            hasData = false;
        }
    }
}


void procFragmentsQueue() {
    bool hasData = true;
    while (hasData == true) {
        processFragments(messageQueue, fragmentQueue);
        
        // check if we are done with file and no more data in input queue
        if ( (fileFinished == true) &&
             (fragmentQueue.empty() == true) )
        {
            hasData = false;
        }
    }
}


void procMessagesQueue() {
    bool hasData = true;
    while (hasData == true) {
        processMessages(payloadQueue, messageQueue);
        
        // check if we are done with file and no more data in input queue
        if ( (fileFinished == true) &&
             (messageQueue.empty() == true) )
        {
            hasData = false;
        }
    }
}


void procPayloadsQueue() {
    bool hasData = true;
    while (hasData == true) {
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
                    
                    float dLat = -(posLat/600000.0f - 37.2) * 150;
                    float dLon = (posLon/600000.0f + 88.2) * 150;
                    
                    int y = (int)((dLat / 90.0 + 0.5) * OUTPUT_HEIGHT);
                    int x = (int)((dLon / 180.0 + 0.5) * OUTPUT_WIDTH);
                    if ((y < OUTPUT_HEIGHT) &&
                        (y >= 0) &&
                        (x < OUTPUT_WIDTH) &&
                        (x >= 0) )
                    {
                        size_t offset = y * OUTPUT_WIDTH + x;
                        image[offset] += 1000;
                        pixelMax = std::max((int)image[offset], pixelMax);
                    }
                }

                msgCount++;
            }
        }
                
        // check if we are done with file and no more data in input queue
        if ( (fileFinished == true) &&
             (payloadQueue.empty() == true) )
        {
            hasData = false;
        }
    }
}


void statusReport() {
    double msgRate = 0;
    auto tsOld = Clock::now();
    
    bool hasData = true;
    while (hasData == true) {
        auto ts = Clock::now();
        auto td = std::chrono::duration_cast<std::chrono::nanoseconds>(ts - tsOld).count() * 1e-09;
        msgRate = msgCount / td;
        tsOld = ts;
        msgCount = 0;
    
        printf("frgq=%d, msgq=%d, pldq=%d, rate=%.2f\n",
               (int)fragmentQueue.size(),
               (int)messageQueue.size(),
               (int)payloadQueue.size(),
               (float)msgRate);
               
        // check if we are done with file and no more data in input queue
        if ( (fileFinished == true) &&
             (payloadQueue.empty() == true) )
        {
            hasData = false;
            printf("done.\n");
            
            for (size_t i = 0; i < image.size(); i++) {
                image[i] = image[i] / (double)pixelMax * ((2 << 16) - 1);
            }
            
            writeTiffFileInt16("test.tiff", OUTPUT_WIDTH, OUTPUT_HEIGHT, (uint16_t*)image.data());
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}



    

int main() {
    
    auto thread1 = std::thread(readFragments);
    auto thread2 = std::thread(procFragmentsQueue);
    auto thread3 = std::thread(procMessagesQueue);
    auto thread4 = std::thread(procPayloadsQueue);
    auto thread5 = std::thread(statusReport);
    
    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();
    thread5.join();
}




