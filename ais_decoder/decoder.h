#ifndef AIS_DECODER_H
#define AIS_DECODER_H

#include <array>
#include <cstring>
#include <string>



const size_t MAX_FRAGMENTS              = 5;
const size_t MAX_CHARS_PER_FRAGMENT     = 82;
const size_t MAX_PAYLOAD_SIZE           = MAX_FRAGMENTS * MAX_CHARS_PER_FRAGMENT * 6 / 8 + 1;

using PayloadArray = std::array<unsigned char, MAX_PAYLOAD_SIZE>;


struct NmeaMsg
{
    void init() {
        m_message.m_pBegin = nullptr;
        m_message.m_pEnd = nullptr;
    }

    bool valid() const {
        return (m_message.m_pBegin != nullptr) &&
               (m_message.m_pEnd != nullptr);
    }
    
    StrRef      m_message;              // whole sentence (starting '$' and '!' removed)
    StrRef      m_talkerId;             // words[0] -- AIVDM
    StrRef      m_payload;              // words[5] -- armoured ASCII payload
    uint8_t     m_uFragmentCount;       // words[1] -- single digit integer
    uint8_t     m_uFragmentNum;         // words[2] -- single digit integer
    uint8_t     m_uMsgId;               // words[3] -- multi-sentence set id
    uint8_t     m_uFillBits;            // words[6] -- single digit integer
    uint8_t     m_uMsgCrc;
};


struct MsgPayload
{
    void init() {m_bitsUsed = 0;}
    bool valid() const {return m_bitsUsed != 0;}
    
    PayloadArray    m_payload;
    uint32_t        m_bitsUsed;
};



// calc message CRC
uint8_t calcCrc(const StrRef &_strMsg)
{
    const unsigned char* in_ptr = (const unsigned char*)_strMsg.data();
    const unsigned char* in_sentinel  = in_ptr + _strMsg.size();
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


// Read sentence from input. Returns the number of bytes read.
size_t readSentence(NmeaMsg *_pMsg, const char *_pInput, size_t _uInputSize) {
    char *pData = (char*)_pInput;
    const char *pEnd = pData + _uInputSize;
    
    if (strncmp(pData, "AIVDM", 5) == 0) {
        _pMsg->m_talkerId.m_pBegin = pData;
        _pMsg->m_talkerId.m_pEnd = pData + 5;
        _pMsg->m_message.m_pBegin = pData; pData += 6;
        _pMsg->m_uFragmentCount = single_digit_strtoi(pData); pData += 2;
        _pMsg->m_uFragmentNum = single_digit_strtoi(pData); pData += 2;
        
        if (*pData != ',') {
            _pMsg->m_uMsgId = double_digit_hex_strtoi(pData); pData += 3;
        }
        else {
            pData += 1;
        }
        
        pData += 3; // skip words[4]
        
        // find payload
        _pMsg->m_payload.m_pBegin = pData;
        pData = (char*)memchr(pData, ',', _uInputSize);
        if (pData == nullptr) {
            return 0;
        }
        
        _pMsg->m_payload.m_pEnd = pData; pData += 2;
        _pMsg->m_message.m_pEnd = pData;
        
        // find CRC
        if (pData >= pEnd) {
            return 0;
        }
        
        _pMsg->m_uMsgCrc = double_digit_hex_strtoi(pData + 1);
        pData += 2;
    }
    
    return pData - _pInput;
}


// Convert payload to decimal (de-armour) and concatenate 6bit decimal values. Returns the payload bits used.
int decodeAscii(MsgPayload *_pPayload, const StrRef &_strPayload, int _iFillBits)
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
    
    const unsigned char* in_ptr = (unsigned char*)_strPayload.data();
    const unsigned char* in_sentinel = in_ptr + _strPayload.size();
    const unsigned char* in_sentinel4 = in_sentinel - 4;
    unsigned char* out_ptr = _pPayload->m_payload.data();
    
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
    
    return (int)(_strPayload.size() * 6 - _iFillBits);
}


/* unpack next _iBits (most significant bit is packed first) */
unsigned int getUnsignedValue(const MsgPayload *_pPayload, size_t &_uBitIndex, int _iBits)
{
    const unsigned char *lptr = _pPayload->m_payload.data() + (_uBitIndex >> 3);
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
int getSignedValue(const MsgPayload *_pPayload, size_t &_uBitIndex, int _iBits)
{
    const unsigned char *lptr = _pPayload->m_payload.data() + (_uBitIndex >> 3);
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
std::string getString(const MsgPayload *_pPayload, size_t &_uBitIndex, int _iBits)
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
        unsigned int ch = getUnsignedValue(_pPayload, _uBitIndex, 6);
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
