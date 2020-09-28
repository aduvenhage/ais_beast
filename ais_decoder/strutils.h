#ifndef AIS_STR_UTILS_H
#define AIS_STR_UTILS_H


#include <string>



const char ASCII_CHARS[]                = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ !\"#$%&'()*+,-./0123456789:;<=>?";


// Reference to a sub-string from String
struct StringRef
{
    StringRef()
        :m_pData(nullptr),
         m_uOffset(0),
         m_uSize(0)
    {}
    
    char      *m_pData;
    size_t    m_uOffset;
    size_t    m_uSize;
};




template <int N>
class String
{
 public:
    String()
        :m_size(0)
    {}
    
    String(const char *_psz) {
        m_size = std::max(m_str.size(), strlen(_psz));
        strncpy(m_str.data(), _psz, m_str.size());
    }
    
    template <int PN>
    String &operator=(const String<PN> &_str) {
        m_size = std::min(m_str.size(), _str.size());
        memcpy(m_str.data(), _str.data(), m_size);
        return *this;
    }
    
    String &operator=(const StringRef &_str) {
        m_size = std::min(m_str.size(), _str.m_uSize);
        memcpy(m_str.data(), _str.m_pData + _str.m_uOffset, m_size);
        return *this;
    }
    
    const char *data() const {return m_str.data();}
    char *data() {return m_str.data();}
    size_t size() const {return m_size;}
    size_t maxSize() const {return N;}
    
    void setSize(size_t _uSize) {
        m_size = std::min(m_str.size(), _uSize);
    }
    
    template <int PN>
    size_t append(const String<PN> &_str) {
        size_t offset = m_size;
        m_size = std::min(m_str.size(), m_size + _str.size());
        memcpy(m_str.data() + offset, _str.data(), m_size - offset);
        return m_size;
    }
    
    size_t append(const char *_psz) {
        size_t offset = m_size;
        m_size = std::min(m_str.size(), m_size + strlen(_psz));
        memcpy(m_str.data() + offset, _psz, m_size - offset);
        return m_size;
    }

    size_t append(const char *_pData, size_t _uSize) {
        size_t offset = m_size;
        m_size = std::min(m_str.size(), m_size + _uSize);
        memcpy(m_str.data() + offset, _pData, m_size - offset);
        return m_size;
    }

    size_t append(const StringRef &_str) {
        return append(_str.m_pData + _str.m_uOffset, _str.m_uSize);
    }

 private:
    std::array<char, N>     m_str;
    size_t                  m_size;
};


template <typename int_type>
int_type bswap64(int_type _i) {
    return __builtin_bswap64(_i);
}


// very fast single digit string to int
inline int single_digit_strtoi(const char *pData)
{
    return (*pData - '0') & 0x0F;
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
