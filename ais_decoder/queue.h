#ifndef AIS_QUEUE_H
#define AIS_QUEUE_H



#include <memory>
#include <atomic>
#include <array>
#include <string>
#include <mutex>



constexpr bool isPowerOf2(int _n) {
    return (_n > 0) && ((_n & (_n-1)) == 0);
}


/* Ring buffer based queue (lock free and thread-safe for single producer / single consumer) */
template <typename payload_type, int N>
class LockFreeQueue
{
 static_assert(isPowerOf2(N), "Queue internal size should be a power of two.");
 protected:
    const size_t    MASK = N-1;
    
 public:
    LockFreeQueue()
        :m_uFront(0),
         m_uBack(0)
    {}
    
    template <typename T>
    bool push(T &&_p) {
        if (full() == true) {
            return false;
        }
        
        m_array[m_uBack & MASK] = std::forward<T>(_p);
        m_uBack++;
        return true;
    }
    
    payload_type &back() {
        return m_array[(m_uBack-1) & MASK];
    }
    
    bool pop(payload_type &_p) {
        if (empty() == true) {
            return false;
        }
        
        _p = std::move(m_array[m_uFront & MASK]);
        m_uFront++;
        return true;
    }
    
    bool empty() const {
        return m_uBack == m_uFront;
    }
    
    bool full() const {
        return m_uBack - m_uFront >= N;
    }
    
 private:
    std::array<payload_type, N>    m_array;
    std::atomic<uint32_t>          m_uFront;
    std::atomic<uint32_t>          m_uBack;
};







#endif // #ifndef AIS_QUEUE_H
