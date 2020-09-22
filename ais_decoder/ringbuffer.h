#ifndef AIS_RING_BUFFER_H
#define AIS_RING_BUFFER_H


#include <array>
#include <atomic>



constexpr bool isPowerOfTwo(int _i) {
    return _i && !(_i & (_i - 1));
}


// lock free queue (ringbuffer), single producer, single consumer
template <typename payload_type, int SIZE>
class RingBuffer
{
 static_assert(isPowerOfTwo(SIZE), "Ring buffer size must be a power of two!");
 private:
    static const size_t SIZE_MASK = SIZE - 1;
 
 public:
    RingBuffer()
        :m_front(0),
         m_back(0)
    {}
 
    // Push a new element and return the reference to it. Returns null if buffer is full.
    payload_type *push() {
        if (m_back - m_front > SIZE-2) {
            return nullptr;
        }
    
        auto pPayload = &m_array[m_back & SIZE_MASK];
        pPayload->init();
        m_back++;
        
        return pPayload;
    }
 
    // pop element off queue. The return value is valid until next pop.
    payload_type *pop() {
        if (m_back - m_front < 2) {
            return nullptr;
        }
        
        auto pPayload = &m_array[m_front & SIZE_MASK];
        m_front++;
        
        if (pPayload->valid() == true) return pPayload;
        else return nullptr;
    }

 private:
    std::array<payload_type, SIZE>      m_array;
    std::atomic<uint32_t>               m_front;
    std::atomic<uint32_t>               m_back;
};



#endif // #ifndef AIS_RING_BUFFER_H
