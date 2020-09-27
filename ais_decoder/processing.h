#ifndef AIS_PROCESSING_H
#define AIS_PROCESSING_H

#include "decoder.h"
#include "queue.h"

#include <memory>


// Fixed size grouping of decoder structures
template <typename payload_type, int N>
struct Chunk
{
    Chunk()
        :m_count(0),
         m_index(0)
    {}

    payload_type &data() {return m_data.data()[m_count];}
    void next() {m_count++;}
    void reset() {m_count = 0; m_index = 0;}
    bool full() const {return m_count >= N;}
    bool empty() const {return m_count == 0;}
    payload_type *begin() {return m_data.data();}
    payload_type *end() {return m_data.data() + m_count;}
    size_t count() const {return m_count;}
    size_t maxSize() const {return (size_t)N;}
    
    std::array<payload_type, N>   m_data;
    size_t                        m_count;      // size used
    size_t                        m_index;      // size processed
};


using Fragments = Chunk<NmeaFrg, 1024 * 2>;
using Messages = Chunk<NmeaMsg, 1024 * 2>;
using Payloads = Chunk<MsgPayload, 1024 * 2>;


/*
    Process NMEA raw input data. Tries to all data in input.
    Stops when output queue is full.
    Returns the number of bytes processed from the input.
 */
template <typename QueueFragments, typename NmeaData>
size_t processNmeaData(QueueFragments &_fragmentQueue, const NmeaData &_nmeaData)
{
    if (_fragmentQueue.full() == true) {
        return 0;
    }
    
    // process data
    char *pData = const_cast<char*>(_nmeaData.data());
    char *pEnd = pData + _nmeaData.size();
    auto pFragments = std::make_unique<Fragments>();

    while (pData < pEnd) {
        // skip '!' and '$'
        pData = (*pData == '!') ? pData + 1 : pData;
        pData = (*pData == '$') ? pData + 1 : pData;

        auto &fragment = pFragments->data();
        size_t n = readSentence(fragment, pData, pEnd - pData);
        if (n > 0) {
            // read EOLs
            pData += n;
            pData = ((pData < pEnd) && (*pData == '\r')) ? pData + 1 : pData;
            pData = ((pData < pEnd) && (*pData == '\n')) ? pData + 1 : pData;
            
            // try to output fragment
            pFragments->next();
            if (pFragments->full() == true) {
                _fragmentQueue.push(std::move(pFragments));
                
                // stop if queue is full, or continue
                if (_fragmentQueue.full() == true) {
                    break;
                }
                else {
                    pFragments = std::make_unique<Fragments>();
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

    // output last fragments
    if (pFragments->empty() == false) {
        _fragmentQueue.push(std::move(pFragments));
    }
    
    return pData - _nmeaData.data();
}


/*
    Process fragments and produce messages. Stops when output queue is full.
    Returns the number of fragments processed.
*/
template <typename QueueMessages, typename QueueFragments>
size_t processFragments(QueueMessages &_messageQueue, QueueFragments &_fragmentQueue)
{
    size_t count = 0;
    if (_messageQueue.full() == false) {
        for (;;) {
            std::unique_ptr<Fragments> pFragments;
            if (_fragmentQueue.pop(pFragments) == false) {
                break;
            }

            auto pMessages = std::make_unique<Messages>();
            auto &fragments = *pFragments;
            for (auto &frg : fragments) {
                auto &message = pMessages->data();
                if (processSentence(message, frg) == true) {
                    pMessages->next();
                    assert(pMessages->full() == false);
                }
                
                count++;
            }

            if (pMessages->empty() == false) {
                _messageQueue.push(std::move(pMessages));
            }
        }
    }
    
    return count;
}


/*
    Process messages and produce decoded payloads.  Stops when output queue is full.
    Returns the number of messages processed.
*/
template <typename QueuePayloads, typename QueueMessages>
size_t processMessages(QueuePayloads &_payloadQueue, QueueMessages &_messageQueue)
{
    size_t count = 0;
    
    if (_payloadQueue.full() == false) {
        for (;;) {
            std::unique_ptr<Messages> pMessages;
            if (_messageQueue.pop(pMessages) == false) {
                break;
            }
                
            auto pPayloads = std::make_unique<Payloads>();
            auto &messages = *pMessages;
            for (auto &msg : messages) {
                auto &payload = pPayloads->data();
                payload.m_bitsUsed = decodeAscii(payload, msg);
                if (payload.m_bitsUsed > 0) {
                    pPayloads->next();
                    assert(pPayloads->full() == false);
                }
                
                count++;
            }

            if (pPayloads->empty() == false) {
                _payloadQueue.push(std::move(pPayloads));
            }
        }
    }
    
    return count;
}







#endif // #ifndef AIS_PROCESSING_H
