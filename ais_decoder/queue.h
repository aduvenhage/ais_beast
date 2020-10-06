#ifndef AIS_QUEUE_H
#define AIS_QUEUE_H



#include <memory>
#include <atomic>
#include <array>
#include <string>
#include <mutex>
#include <condition_variable>

using namespace std::literals::chrono_literals;


constexpr bool isPowerOf2(int _n) {
    return (_n > 0) && ((_n & (_n-1)) == 0);
}


/*
   Ring buffer based queue (thread-safe for multiple producers and consumers).
   Pop and push operations may block.
 */
template <typename payload_type, int N>
class BlockingQueue
{
 static_assert(isPowerOf2(N), "Queue internal size should be a power of two.");
 protected:
    const size_t            MASK = N-1;
    
 public:
    BlockingQueue()
        :m_uSize(0),
         m_uFront(0),
         m_uBack(0)
    {}
    
    template <typename T>
    bool push(T &&_p) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&]{return !full();});

        m_array[m_uBack & MASK] = std::forward<T>(_p);
        m_uBack++;
        m_uSize = m_uBack - m_uFront;

        lock.unlock();
        m_cv.notify_one();
        return true;
    }
    
    bool pop(payload_type &_p, const std::chrono::milliseconds &_timeout = 5000ms) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_cv.wait_for(lock, _timeout, [&]{return !empty();}) == false) {
            return false;
        }

        _p = std::move(m_array[m_uFront & MASK]);
        m_uFront++;
        m_uSize = m_uBack - m_uFront;

        lock.unlock();
        m_cv.notify_one();
        return true;
    }
    
    payload_type pop(const std::chrono::milliseconds &_timeout = 5000ms) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_cv.wait_for(lock, _timeout, [&]{return !empty();}) == false) {
            return payload_type{};
        }

        auto p = std::move(m_array[m_uFront & MASK]);
        m_uFront++;
        m_uSize = m_uBack - m_uFront;

        lock.unlock();
        m_cv.notify_one();
        return p;
    }
    
    bool empty() const {
        return m_uSize == 0;
    }
    
    bool full() const {
        return m_uSize >= N;
    }
    
    size_t size() const {
        return m_uSize;
    }
    
 private:
    std::array<payload_type, N>    m_array;
    std::mutex                     m_mutex;
    std::condition_variable        m_cv;
    std::atomic<uint32_t>          m_uSize;
    uint32_t                       m_uFront;
    uint32_t                       m_uBack;
};



#endif // #ifndef AIS_QUEUE_H
