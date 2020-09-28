#ifndef AIS_PROCESSING_H
#define AIS_PROCESSING_H

#include "chunk.h"
#include "decoder.h"
#include "queue.h"

#include <memory>


const size_t AIS_CHUNK_SIZE = 512;
using Fragments = Chunk<NmeaFrg, AIS_CHUNK_SIZE>;
using Messages = Chunk<NmeaMsg, AIS_CHUNK_SIZE>;
using Payloads = Chunk<MsgPayload, AIS_CHUNK_SIZE>;


/*
    Process NMEA raw input data.
    Processes only one chunk of data at a time.
    Returns the number of bytes processed from the input.
 */
template <typename QueueFragments, typename NmeaData>
size_t processNmeaData(QueueFragments &_fragmentQueue, const NmeaData &_nmeaData)
{
    // process data
    char *pData = const_cast<char*>(_nmeaData.data());
    char *pEnd = pData + _nmeaData.size();
    std::unique_ptr<Fragments> pFragments;
    
    while (pData < pEnd) {
        // stop if queue is full
        if (_fragmentQueue.full() == true) {
            break;
        }
        
        if (pFragments == nullptr) {
            pFragments = std::make_unique<Fragments>();
        }
        
        // TODO: process header and footer on special formats

        // skip '!' and '$'
        pData = (*pData == '!') ? pData + 1 : pData;
        pData = (*pData == '$') ? pData + 1 : pData;

        auto &fragment = pFragments->push_back();
        size_t n = readSentence(fragment, pData, pEnd - pData);
        if (n > 0) {
            // read EOLs
            pData += n;
            pData = ((pData < pEnd) && (*pData == '\r')) ? pData + 1 : pData;
            pData = ((pData < pEnd) && (*pData == '\n')) ? pData + 1 : pData;
            
            // try to output full chunk
            if (pFragments->full() == true) {
                _fragmentQueue.push(std::move(pFragments));
            }
        }
        else {
            // nothing read, so rewind
            pFragments->pop_back();
            
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

    // output last chunk
    if ( (pFragments != nullptr) &&
         (pFragments->empty() == false) )
    {
        _fragmentQueue.push(std::move(pFragments));
    }
    
    return pData - _nmeaData.data();
}


/*
    Process fragments and produce messages.
    Stops when output queue is full.
    Returns the number of fragments processed.
*/
template <typename QueueMessages, typename QueueFragments>
size_t processFragments(QueueMessages &_messageQueue, QueueFragments &_fragmentQueue)
{
    size_t count = 0;
    for (int i = 0; i < 128; i++) {
        if (_messageQueue.full() == true) {
            break;
        }
        
        auto pFragments = _fragmentQueue.pop();
        if (pFragments == nullptr) {
            break;
        }

        auto pMessages = std::make_unique<Messages>();
        auto &fragments = *pFragments;
        for (auto &frg : fragments) {
            assert(pMessages->full() == false);
            auto &message = pMessages->push_back();
            if (processSentence(message, frg) == false) {
                pMessages->pop_back();
            }
            
            count++;
        }

        if ( (pMessages != nullptr) &&
             (pMessages->empty() == false) )
        {
            _messageQueue.push(std::move(pMessages));
        }
    }
    
    return count;
}


/*
    Process messages and produce decoded payloads.
    Stops when output queue is full.
    Returns the number of messages processed.
*/
template <typename QueuePayloads, typename QueueMessages>
size_t processMessages(QueuePayloads &_payloadQueue, QueueMessages &_messageQueue)
{
    size_t count = 0;
    for (int i = 0; i < 128; i++) {
        if (_payloadQueue.full() == true) {
            break;
        }
        
        auto pMessages = _messageQueue.pop();
        if (pMessages == nullptr) {
            break;
        }
            
        auto pPayloads = std::make_unique<Payloads>();
        auto &messages = *pMessages;
        for (auto &msg : messages) {
            assert(pPayloads->full() == false);
            auto &payload = pPayloads->push_back();
            if (decodeAscii(payload, msg) == 0) {
                // nothing decoded, so rewind
                pPayloads->pop_back();
            }
            
            count++;
        }

        if ( (pPayloads != nullptr) &&
             (pPayloads->empty() == false) )
        {
            _payloadQueue.push(std::move(pPayloads));
        }
    }
    
    return count;
}







#endif // #ifndef AIS_PROCESSING_H
