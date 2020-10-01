#ifndef AIS_POOL_H
#define AIS_POOL_H



#include <memory>
#include <mutex>
#include <vector>



<<<<<<< HEAD
/* Stack like pool of memory for a single type. */
=======
>>>>>>> 3db7dbd1d6a2822f0d5808d54bb5f20f46d3ad72
template <typename obj_type>
class MemoryPool
{
 public:
    ~MemoryPool() {
        for (auto *p : m_pool) {
            free(p);
        }
    }
    
    static MemoryPool &instance() {
        static MemoryPool thePool;
        return thePool;
    }
    
<<<<<<< HEAD
    // allocate or find available objects
=======
>>>>>>> 3db7dbd1d6a2822f0d5808d54bb5f20f46d3ad72
    static void *getObjectPtr() {
        return instance().findObjectPtr();
    }
    
<<<<<<< HEAD
    // put object back in pool
=======
>>>>>>> 3db7dbd1d6a2822f0d5808d54bb5f20f46d3ad72
    static void releaseObjectPtr(void *_p) {
        return instance().returnObjectPtr(_p);
    }
    
 private:
    void *findObjectPtr() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_pool.empty() == true) {
            return malloc(sizeof(obj_type));
        }
        else {
            void *p = m_pool.back();
            m_pool.pop_back();
            return p;
        }
    }
<<<<<<< HEAD

=======
    
>>>>>>> 3db7dbd1d6a2822f0d5808d54bb5f20f46d3ad72
    void returnObjectPtr(void *_p) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pool.push_back(_p);
    }
    
 private:
    std::vector<void*>   m_pool;
    std::mutex           m_mutex;
};




#endif // #ifndef AIS_POOL_H
