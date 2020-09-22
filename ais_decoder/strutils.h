#ifndef AIS_STR_UTILS_H
#define AIS_STR_UTILS_H


#include <string>



const char ASCII_CHARS[]                = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&'()*+,-./0123456789:;<=>?";

struct StrRef
{
    const char *data() const {return m_pBegin;}
    char *data() {return m_pBegin;}
    size_t size() const {return m_pEnd - m_pBegin;}
    
    char *m_pBegin;
    char *m_pEnd;
};


template <typename int_type>
int_type bswap64(int_type _i) {
    return __builtin_bswap64(_i);
}


// very fast single digit string to int
inline int single_digit_strtoi(const char *pData)
{
    return (*pData - '0') & 0x09;
}


// very double digit hex string to int (only works for upper case HEX)
inline int double_digit_hex_strtoi(const char *pData)
{
    static const uint8_t hex_t[32] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0,
        10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    return (hex_t[(uint8_t)(pData[0] - '0') & 0x1F] << 4) +
           hex_t[(uint8_t)(pData[1] - '0') & 0x1F];
}


// quick check for asscii space characters
inline bool ascii_isspace(char _ch)
{
    return (_ch == ' ') || (_ch == '\t') || (_ch == '\n') || (_ch == '\r');
}


#endif // #ifndef AIS_STR_UTILS_H
