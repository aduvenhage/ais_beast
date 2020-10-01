#ifndef AIS_CHUNK_H
#define AIS_CHUNK_H

#include "mem_pool.h"
 

/*
    Fixed size array of decoder structures. Interface allows sequential access to data.
    Overloaded new/delelete memory operators allows for better performance optimisation.
 */
template <typename payload_type, int N>
struct Chunk
{
    Chunk()
        :m_size(0)
    {}

    payload_type &back() {
        return m_data.data()[m_size-1];
    }
    
    payload_type &push_back() {
        m_size += m_size < N ? 1 : 0;
        return back();
    }

    void pop_back() {
        m_size -= m_size > 0 ? 1 : 0;
    }

    void clear() {
        m_size = 0;
    }

    bool full() const {
        return m_size >= N;
    }

    bool empty() const {
        return m_size == 0;
    }

    payload_type *begin() {
        return m_data.data();
    }

    payload_type *end() {
        return m_data.data() + m_size;
    }

    size_t size() const {
        return m_size;
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
    size_t                        m_size;
};






#endif // #ifndef AIS_CHUNK_H

