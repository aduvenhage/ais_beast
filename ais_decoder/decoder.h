#ifndef AIS_DECODER_H
#define AIS_DECODER_H

#include "strutils.h"
#include "mem_pool.h"

#include <array>
#include <cstring>
#include <string>


const size_t MAX_FRAGMENTS              = 3;
const size_t MAX_CHARS_PER_FRAGMENT     = 82;
const size_t MAX_PAYLOAD_SIZE           = MAX_FRAGMENTS * MAX_CHARS_PER_FRAGMENT * 6 / 8 + 1;

using FrgStr = String<MAX_CHARS_PER_FRAGMENT>;
using MsgStr = String<MAX_PAYLOAD_SIZE>;
using PayloadArray = std::array<unsigned char, MAX_PAYLOAD_SIZE>;


// TODO: overload new and delete and reuse objects for frg, msg & payload

struct NmeaFrg
{
    FrgStr      m_sentence;             // whole sentence (starting '$' and '!' removed)
    FrgStr      m_payload;              // words[5] -- armoured ASCII payload
    uint8_t     m_uFragmentCount;       // words[1] -- single digit integer
    uint8_t     m_uFragmentNum;         // words[2] -- single digit integer
    uint8_t     m_uMsgId;               // words[3] -- multi-sentence set id
    uint8_t     m_uChannelId;           // words[4] -- channel id character value
    uint8_t     m_uFillBits;            // words[6] -- single digit integer
    uint8_t     m_uMsgCrc;
};


struct NmeaMsg
{
    MsgStr      m_message;              // whole sentence (starting '$' and '!' removed)
    MsgStr      m_payload;              // words[5] -- armoured ASCII payload
    uint8_t     m_uChannelId;           // words[4] -- channel id character value
    uint8_t     m_uFillBits;            // words[6] -- single digit integer
};


struct MsgPayload
{
    PayloadArray    m_payload;
    uint32_t        m_bitsUsed;
};


/* calc message CRC */
template <int N>
uint8_t calcCrc(const String<N> &_str)
{
    const unsigned char* in_ptr = (const unsigned char*)_str.data();
    const unsigned char* in_sentinel  = in_ptr + _str.size() - 3;
    const unsigned char* in_sentinel4 = in_sentinel - 4;
    
    uint8_t checksum = 0;
    while ((intptr_t(in_ptr) & 3) && in_ptr < in_sentinel) {
        checksum ^= *in_ptr++;
    }
    
    uint32_t checksum4 = checksum;
    while (in_ptr < in_sentinel4) {
        checksum4 ^= *((uint32_t*)in_ptr);
        in_ptr += 4;
    }
    
    checksum = (checksum4 & 0xff) ^ ((checksum4 >> 8) & 0xff) ^ ((checksum4 >> 16) & 0xff) ^ ((checksum4 >> 24) & 0xff);
    
    while (in_ptr < in_sentinel) {
        checksum ^= *in_ptr++;
    }
    
    return checksum;
}


/* Read sentence from input. Returns the number of bytes read. */
size_t readSentence(NmeaFrg &_frg, const char *_pInput, size_t _uInputSize) {
    const char *pBegin = _pInput;
    const char *pEnd = _pInput + _uInputSize;
    char *pData = (char*)pBegin;
    
    // TODO: check agains multiple talker strings
    if (strncmp(pData, "AIVDM", 5) == 0) {
         pData += 6;
        _frg.m_uFragmentCount = single_digit_strtoi(pData); pData += 2;
        _frg.m_uFragmentNum = single_digit_strtoi(pData); pData += 2;
        
        if (*pData != ',') {
            _frg.m_uMsgId = single_digit_strtoi(pData);
            pData += 2;
        }
        else {
            pData += 1;
        }
        
        if (*pData != ',') {
            _frg.m_uChannelId = *((unsigned char*)pData);
             pData += 2;
        }
        else {
            pData += 1;
        }
        
        // find payload
        const char *pPayload = pData;
        pData = (char*)memchr(pData, ',', _uInputSize);
        if (pData == nullptr) {
            return 0;
        }

        _frg.m_payload.append(pPayload, pData - pPayload); pData += 2;
        
        // find CRC
        if (pData + 4 >= pEnd) {
            return 0;
        }
        
        _frg.m_uMsgCrc = double_digit_hex_strtoi(pData + 1);
        pData += 3;
        
        // copy sentence
        _frg.m_sentence.append(pBegin, pData - pBegin);
    }
    
    return pData - _pInput;
}


/* Process one sentence/fragment and possibly produce a message. Returns true if a full message was decoded. */
bool processSentence(NmeaMsg &_msg, const NmeaFrg &_frg)
{
    static thread_local std::array<NmeaMsg, 10> messages;
    
    // check sentence CRC
    uint8_t crc = calcCrc(_frg.m_sentence);
    if (crc != _frg.m_uMsgCrc) {
        return false;
    }

    if (_frg.m_uFragmentCount == 1) {
        _msg.m_message = _frg.m_sentence;
        _msg.m_payload = _frg.m_payload;
        _msg.m_uChannelId = _frg.m_uChannelId;
        _msg.m_uFillBits = _frg.m_uFillBits;
        
        return true;
    }
    else {
    
    }
    
    return false;
}


/* Convert payload to decimal (de-armour) and concatenate 6bit decimal values. Returns the payload bits used. */
int decodeAscii(MsgPayload &_payload, const NmeaMsg &_msg)
{
    static const unsigned char dLUT[256] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
        17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    
    const unsigned char* in_ptr = (unsigned char*)_msg.m_payload.data();
    const unsigned char* in_sentinel = in_ptr + _msg.m_payload.size();
    const unsigned char* in_sentinel4 = in_sentinel - 4;
    unsigned char* out_ptr = _payload.m_payload.data();
    
    uint64_t accumulator = 0;
    unsigned int acc_bitcount = 0;
    
    while (in_ptr < in_sentinel4) {
        uint64_t val = (dLUT[*in_ptr] << 18) | (dLUT[*(in_ptr+1)] << 12) | (dLUT[*(in_ptr+2)] << 6) | dLUT[*(in_ptr+3)];
        
        constexpr unsigned int nbits = 24;
        
        int remainder = int(64 - acc_bitcount) - nbits;
        if (remainder <= 0) {
            // accumulator will fill up, commit to output buffer
            accumulator |= (uint64_t(val) >> -remainder);
            *((uint64_t*)out_ptr) = bswap64(accumulator);
            out_ptr += 8;
            
            if (remainder < 0) {
                accumulator = uint64_t(val) << (64 + remainder); // remainder is negative
                acc_bitcount = -remainder;
            } else {
                accumulator = 0;  // shifting right by 64 bits (above) does not yield zero?
                acc_bitcount = 0;
            }
            
        } else {
            // we still have enough room in the accumulator
            accumulator |= uint64_t(val) << (64 - acc_bitcount - nbits);
            acc_bitcount += nbits;
        }
        
        in_ptr += 4;
    }
    
    while (in_ptr < in_sentinel) {
        uint64_t val = dLUT[*in_ptr];
        
        constexpr unsigned int nbits = 6;
        
        int remainder = int(64 - acc_bitcount) - nbits;
        if (remainder <= 0) {
            // accumulator will fill up, commit to output buffer
            accumulator |= (uint64_t(val) >> -remainder);
            *((uint64_t*)out_ptr) = bswap64(accumulator);
            out_ptr += 8;
            
            if (remainder < 0) {
                accumulator = uint64_t(val) << (64 + remainder); // remainder is negative
                acc_bitcount = -remainder;
            } else {
                accumulator = 0;  // shifting right by 64 bits (above) does not yield zero?
                acc_bitcount = 0;
            }
            
        } else {
            // we still have enough room in the accumulator
            accumulator |= uint64_t(val) << (64 - acc_bitcount - nbits);
            acc_bitcount += nbits;
        }
        
        in_ptr++;
    }
    *((uint64_t*)out_ptr) = bswap64(accumulator);
    
    return (int)(_msg.m_payload.size() * 6 -_msg.m_uFillBits);
}


/* unpack next _iBits (most significant bit is packed first) */
unsigned int getUnsignedValue(const MsgPayload &_payload, size_t &_uBitIndex, int _iBits)
{
    const unsigned char *lptr = _payload.m_payload.data() + (_uBitIndex >> 3);
    uint64_t bits = (uint64_t)lptr[0] << 40;
    bits |= (uint64_t)lptr[1] << 32;
    
    if (_iBits > 9) {
        bits |= (unsigned int)lptr[2] << 24;
        bits |= (unsigned int)lptr[3] << 16;
        bits |= (unsigned int)lptr[4] << 8;
        bits |= (unsigned int)lptr[5];
    }
    
    bits <<= 16 + (_uBitIndex & 7);
    _uBitIndex += _iBits;

    return (unsigned int)(bits >> (64 - _iBits));
}


/* unpack next _iBits (most significant bit is packed first; with sign check/conversion) */
int getSignedValue(const MsgPayload &_payload, size_t &_uBitIndex, int _iBits)
{
    const unsigned char *lptr = _payload.m_payload.data() + (_uBitIndex >> 3);
    uint64_t bits = (uint64_t)lptr[0] << 40;
    bits |= (uint64_t)lptr[1] << 32;
    
    if (_iBits > 9) {
        bits |= (unsigned int)lptr[2] << 24;
        bits |= (unsigned int)lptr[3] << 16;
        bits |= (unsigned int)lptr[4] << 8;
        bits |= (unsigned int)lptr[5];
    }
    
    bits <<= 16 + (_uBitIndex & 7);
    _uBitIndex += _iBits;

    return (int)((int64_t)bits >> (64 - _iBits));
}


/* unback string (6 bit characters) -- already cleans string (removes trailing '@' and trailing spaces) */
std::string getString(const MsgPayload &_payload, size_t &_uBitIndex, int _iBits)
{
    static thread_local std::array<char, 64> strdata;
    
    int iNumChars = _uBitIndex/6;
    if (iNumChars > (int)strdata.size())
    {
        iNumChars = (int)strdata.size();
    }
    
    int32_t iStartBitIndex = _uBitIndex;
    
    for (int i = 0; i < iNumChars; i++)
    {
        unsigned int ch = getUnsignedValue(_payload, _uBitIndex, 6);
        if (ch > 0) // stop on '@'
        {
            strdata[i] = ASCII_CHARS[ch];
        }
        else
        {
            iNumChars = i;
            break;
        }
    }
    
    // remove trailing spaces
    while (iNumChars > 0)
    {
        if (ascii_isspace(strdata[iNumChars-1]) == true)
        {
            iNumChars--;
        }
        else
        {
            break;
        }
    }
    
    // make sure bit index is correct
    _uBitIndex = iStartBitIndex + _iBits;
    
    return std::string(strdata.data(), iNumChars);
}





#endif // #ifndef AIS_DECODER_H
