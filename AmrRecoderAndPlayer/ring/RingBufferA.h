#ifndef RINGBUFFERA_HEAD
#define RINGBUFFERA_HEAD
#include <memory>

typedef struct ChunkInfo* ChunkInfoRef;
typedef struct PutBufferChunk* PutBufferChunkRef;
typedef struct PopBufferChunk* PopBufferChunkRef;

enum PopbufState
{
    e_start,
    e_partial,
    e_end,
    e_whole
};

typedef void (*PutCallBackFun)(const PutBufferChunkRef, void* userData);
typedef void (*PopCallBackFun)(ChunkInfoRef, void* userData, PopbufState state, unsigned fillneedSize);

struct ChunkInfo
{
    unsigned       m_capacity;
    unsigned char* m_data;
    unsigned       m_dataSize;
};


struct PutBufferChunk : public ChunkInfo
{
    PutCallBackFun    m_callback;
    void*          m_userData;
};

struct PopBufferChunk : public ChunkInfo
{
    PopCallBackFun   m_callback;
    void*          m_userData;
    int            m_fillDataSize;
};



class RingBufferA_context;
class RingBufferA
{
public:
    RingBufferA(unsigned ringBufferCapacity, unsigned ringBufferCount);
#ifdef _ice_verbose
    RingBufferA(unsigned ringBufferCapacity, unsigned ringBufferCount, const char* MutexName, bool showRingLockinfo, bool showBufferLockInfo);
#endif

    ~RingBufferA();
    unsigned char* get(unsigned& limitCapacity, size_t waitMicroSeconds = 0);
    void  put(unsigned filledSize);
    bool  pop(PopBufferChunkRef chunkref, size_t waitMicroSeconds = 0);
    void  flush();
    void  freeBuffers();
private:
    std::auto_ptr<RingBufferA_context> _ctx;
};

#endif
