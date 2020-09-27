#ifndef AIS_CHUNK_H
#define AIS_CHUNK_H

#include "mem_pool.h"
 

// Fixed size grouping of decoder structures
template <typename payload_type, int N>
struct Chunk
{
    Chunk()
        :m_count(0),
         m_index(0)
    {}

    payload_type &data() {
        return m_data.data()[m_count];
    }
    
    void next() {
        m_count++;
    }

    void reset() {
        m_count = 0;
        m_index = 0;
    }

    bool full() const {
        return m_count >= N;
    }

    bool empty() const {
        return m_count == 0;
    }

    payload_type *begin() {
        return m_data.data();
    }

    payload_type *end() {
        return m_data.data() + m_count;
    }

    size_t count() const {
        return m_count;
    }

    size_t maxSize() const {
        return (size_t)N;
    }

    void *operator new(size_t) {
        return MemoryPool<Chunk>::getObjectPtr();
    }
    
    void operator delete(void *_p) {
        MemoryPool<Chunk>::releaseObjectPtr(_p);
    }
    
    std::array<payload_type, N>   m_data;
    size_t                        m_count;      // size used
    size_t                        m_index;      // size processed
};






#endif // #ifndef AIS_CHUNK_H

