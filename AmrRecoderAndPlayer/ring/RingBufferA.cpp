#include "RingBufferA.h"
#include <IceUtil/Monitor.h>
#include <IceUtil/Mutex.h>
#include <IceUtil/Time.h>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <list>
#include <algorithm>
#include <IceUtil/Thread.h>
#include <IceUtil/RecMutex.h>
using namespace IceUtil;
using namespace std;



//#define _verbose



/////////////

//#ifdef _ice_verbose
int ext_IceVerboseLevel = 0;
int ext_IceVerboseMutexIndex = 1000;
//#endif
class BufferPool_context
{
public:
#ifdef _ice_verbose
    BufferPool_context(string mutexName, bool showLockInfo);
#endif
    BufferPool_context();
    ~BufferPool_context();
//    bool pop(size_t microSeconds = 0);
    bool pop(PopBufferChunkRef chunkref, size_t microSeconds);
    void put(const PutBufferChunkRef chunkref);
    PutBufferChunkRef allocateBuffer(unsigned chunksize);
    void free(PutBufferChunkRef);
    void freeAll();
    void flush();
private:
    Monitor<RecMutex> _monitor;
    list<PutBufferChunkRef> _chunks;
    list<PutBufferChunkRef> _allocChunks;
    list<PutBufferChunkRef> _swapChunklist;

    size_t  _filledBufIndex;
    size_t  _remainedSize;
    int     _unuseddatasize;
    int     _waittingDatasize;
    size_t  _partialChunkBytesLeft;
    PutBufferChunkRef  _partialUseChunk;
    

};

#ifdef _ice_verbose
BufferPool_context::BufferPool_context(string mutexName, bool showLockInfo)
:_monitor(mutexName.c_str(), showLockInfo)
{
    _filledBufIndex = 0;
    _remainedSize = 0;
    _unuseddatasize  = 0;
    _partialChunkBytesLeft = 0;
    _partialUseChunk = 0;
    _waittingDatasize = 0;

}
#endif


BufferPool_context::BufferPool_context()
{
    _filledBufIndex = 0;
    _remainedSize = 0;
    _unuseddatasize  = 0;
    _partialChunkBytesLeft = 0;
    _partialUseChunk = 0;

    _waittingDatasize = 0;
    //
}

BufferPool_context::~BufferPool_context()
{
    freeAll();
}

void BufferPool_context::freeAll()
{
    for(list<PutBufferChunkRef>::iterator it = _allocChunks.begin(); it != _allocChunks.end(); ++it)
        ::free( (*it)->m_data);
    _allocChunks.clear();
}



template <class T>
void displayHexBin(const T& v, bool hex = true)
{
    const unsigned char c2h[] = "0123456789ABCDEF";
    const unsigned char c2b[] = "01";

    unsigned char* p = (unsigned char*)&v;
    char* buf = new char [sizeof(T)*2+1];
    char* ptmp = buf;
    if(hex)
    {
        p = p + sizeof(T)-1;
        for (size_t i = 0; i < sizeof(T); i++, --p)
        {
            *buf++ = c2h[*p >> 4];
            *buf++ = c2h[*p & 0x0F];
        }
        *buf = '\0';
        //printf("hex format displayed as %s\n", ptmp);
        printf("%s", ptmp);

        delete [] ptmp;

    }
    else
    {
        p = (unsigned char*)&v;
        p = p + sizeof(T)-1;
        ptmp = buf = new char [sizeof(T)*8+1];
        for (size_t i = 0; i < sizeof(T); i++, --p)
        {
            for (int j = 0; j < 8; j++)
                *buf++ = c2b[(*p >> (7-j)) & 0x1];
        }
        *buf = '\0';
        //printf("bin format displayed as %s\n", ptmp);
        printf("%s", ptmp);
        delete [] ptmp;
    }

}

//FILE *recodeFile2 = 0;
//unsigned char filedata2[1000];
void BufferPool_context::put(const PutBufferChunkRef chunkref)
{

////////////////////////////////////////////////////////////////////////////

//        if(recodeFile2 == 0)
//            recodeFile2 = fopen("22050_16 _copy 2.pcm", "rb");
//        fread(filedata2, chunkref->m_dataSize, 1, recodeFile2);
//        if(memcmp(filedata2, chunkref->m_data, chunkref->m_dataSize) != 0)
//        {
//            perror("wrong data!!\n");
//            printf("filebuf:\n");
//            char* buf = (char *)filedata2;
//            for(unsigned i = 0; i<chunkref->m_dataSize; ++i)
//            {
//                displayHexBin(buf[i]);
//                printf(" ");
//            }
//            printf("\n");
//
//
//            buf = (char *)chunkref->m_data;
//            for(unsigned i = 0; i<chunkref->m_dataSize; ++i)
//            {
//                displayHexBin(buf[i]);
//                printf(" ");
//            }
//            printf("\n");
//        }

////////////////////////////////////////////////////////////////////////////

    //lockB
    Monitor<RecMutex>::Lock lock(_monitor);
    if(find(_allocChunks.begin(), _allocChunks.end(), chunkref) == _allocChunks.end() )
    {
        return;
    }

    _chunks.push_front(chunkref);
    _unuseddatasize += chunkref->m_dataSize;
    if( ((_waittingDatasize > 0)  && (_unuseddatasize >= _waittingDatasize))
    ||  ((_waittingDatasize == -1) && (_chunks.size() == 1) ) )
    {
        //cout << "notify !!!!!" << endl;
        _monitor.notify();
    }
}



bool BufferPool_context::pop(PopBufferChunkRef chunkref, size_t microSeconds)
{
    const unsigned buffersize = chunkref->m_fillDataSize;
    PutBufferChunkRef oneWholeChunk = NULL;
    int bytes = 0;
    size_t chunks = 0;
    //pop one
    PutBufferChunkRef lastchunk = NULL;
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        _waittingDatasize = buffersize;

        while( ((_unuseddatasize < _waittingDatasize ) && (_waittingDatasize > 0))
            || (_chunks.empty() && _waittingDatasize == -1) )
        {
            if(0 >= microSeconds)
                _monitor.wait();
            else
            {
                //printf("time waiting %d ms\n", microSeconds);
                if(!_monitor.timedWait(IceUtil::Time::microSeconds(microSeconds)) )
                    return false;
            }

        }


        if(_waittingDatasize == -1)
        {
            //pop out the first one
            oneWholeChunk = _chunks.back();
            _chunks.pop_back();
            _waittingDatasize = 0;
        }
        else
        {
            _unuseddatasize -= buffersize;
            _waittingDatasize = 0;

            //use these data

            if(_partialChunkBytesLeft < buffersize)
            {
                size_t bytesneeded = buffersize - _partialChunkBytesLeft;
                bytes = bytesneeded;

                chunks = 0;
                list<PutBufferChunkRef>::reverse_iterator it = _chunks.rbegin();
                for(; it != _chunks.rend() ; ++it)
                {
                    _swapChunklist.push_front(*it);
                    bytes -= (*it)->m_dataSize;
                    chunks++;
                    if(bytes <= 0)
                    {
                        lastchunk = *it;
                        if(bytes < 0)
                            bytes += (*it)->m_dataSize;  //part of the chunk
                        else
                            bytes = -1;     //indicate the whole chunk
                        break;
                    }
                }

                //remove
                for(size_t i=0; i<chunks; ++i)
                    _chunks.pop_back();
            }
        }
    }



    ////////////////////////////////////for debug ///////////////////////////////////////////////////////
//    static int entertimes = 0;
//    if(++entertimes == 10)
//    {
//        perror("\n");
//    }
//    printf("entertimes %d\n", entertimes);



    ////////////////////////////////////for debug ///////////////////////////////////////////////////////

    ////////////////////////////////////one chunk ///////////////////////////////////////////////////////
    if(chunkref->m_fillDataSize == -1)
    {
        (chunkref->m_callback)(oneWholeChunk, chunkref->m_userData, e_whole, 0);
        (oneWholeChunk->m_callback)(oneWholeChunk, oneWholeChunk->m_userData);
        return true;
    }



    //else
    ////////////////////////////////////specific datasize///////////////////////////////////////////////////////
    if(_partialChunkBytesLeft < buffersize)
    {
        //deal with callbacks
        //partial callback
        if(_partialChunkBytesLeft > 0)
        {
            //partial callbacks

            //pop callback
            //size_t offset = _partialUseChunk->m_dataSize - _partialChunkBytesLeft;
            chunkref->m_data =  _partialUseChunk->m_data + _partialUseChunk->m_dataSize - _partialChunkBytesLeft;
            chunkref->m_dataSize = _partialChunkBytesLeft;

            (chunkref->m_callback)(chunkref, chunkref->m_userData, e_start, buffersize);
            //put callback
             (_partialUseChunk->m_callback)(_partialUseChunk, _partialUseChunk->m_userData);
        }

        //chunks callbacks
        //mid chunks callbacks, both  pop, put callbacks
        list<PutBufferChunkRef>::reverse_iterator itswap =  _swapChunklist.rbegin();
        PopbufState midchunk_state;
        for(size_t t=0; t < chunks-1; ++t)
        {
            if(_partialChunkBytesLeft == 0 && t == 0)
                midchunk_state = e_start;
            else
                midchunk_state = e_partial;
            //pop callbacks

            (chunkref->m_callback)(*itswap, chunkref->m_userData, midchunk_state, buffersize);
            //put callbacks
            ((*itswap)->m_callback)(*itswap, (*itswap)->m_userData);
            ++itswap;
        }
        _swapChunklist.clear();

        //last chunk callbacks

        //deal with state
        PopbufState lastchunk_state;

        if(_partialChunkBytesLeft > 0)
            lastchunk_state = e_end;
        else
        {
            if(chunks == 1)
                lastchunk_state = e_whole;
            else
                lastchunk_state = e_end;
        }
        //pop chunk size
        size_t popdatasize;
        if(bytes== -1)
            popdatasize = lastchunk->m_dataSize;
        else
            popdatasize = bytes;
        //pop callback
        chunkref->m_data = lastchunk->m_data;
        chunkref->m_dataSize = popdatasize;
        (chunkref->m_callback)(chunkref, chunkref->m_userData, lastchunk_state, buffersize);
        //weather need put callback
        if(bytes == -1)
            (lastchunk->m_callback)(lastchunk, lastchunk->m_userData);

        //deal with partial
        if(bytes == -1)
        {
            _partialUseChunk = 0;
            _partialChunkBytesLeft = 0;
        }
        else
        {
            _partialUseChunk = lastchunk;
            _partialChunkBytesLeft = lastchunk->m_dataSize - bytes;

        }
    }
    else
    {
        //printf("partial whole offset:%d datasize:%d\n", _partialUseChunk->m_dataSize-_partialChunkBytesLeft, buffersize);
        chunkref->m_data = _partialUseChunk->m_data
                                                + _partialUseChunk->m_dataSize
                                                - _partialChunkBytesLeft;
        chunkref->m_dataSize = buffersize;
        (chunkref->m_callback)(chunkref, chunkref->m_userData, e_whole, buffersize);

        _partialChunkBytesLeft -= buffersize;

        if(_partialChunkBytesLeft == 0)
        {
            (_partialUseChunk->m_callback)(_partialUseChunk, _partialUseChunk->m_userData);
            _partialUseChunk = 0;
        }
    }
    return true;
}


void BufferPool_context::flush()
{
    Monitor<RecMutex>::Lock lock(_monitor);
    if(_partialUseChunk != NULL)
    {
        _unuseddatasize -= _partialChunkBytesLeft;
        (_partialUseChunk->m_callback)(_partialUseChunk, _partialUseChunk->m_userData);
        _partialUseChunk = NULL;
        _partialChunkBytesLeft = 0;
    }
    for(list<PutBufferChunkRef>::reverse_iterator it = _chunks.rbegin();  it != _chunks.rend(); ++it)
    {
        _unuseddatasize -= (*it)->m_dataSize;
        ((*it)->m_callback)(*it, (*it)->m_userData);
    }

    _chunks.clear();
}

PutBufferChunkRef BufferPool_context::allocateBuffer(unsigned chunksize)
{
    PutBufferChunkRef ref = new PutBufferChunk;
    bzero(ref, sizeof(PutBufferChunk) );
    ref->m_capacity = chunksize;
    ref->m_data =  (unsigned char*)malloc(chunksize);
    _allocChunks.push_back(ref);

    return ref;
}


void BufferPool_context::free(PutBufferChunkRef ref)
{
    if(_allocChunks.end() == find(_allocChunks.begin(), _allocChunks.end(), ref))
        return;
    else
    {
        ::free(ref->m_data);
        _allocChunks.remove(ref);
    }
}








////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class RingBufferA_context
{
    friend class RingBufferA;
public:
#ifdef _ice_verbose
    RingBufferA_context(unsigned ringBufferCapacity, unsigned ringBufferCount, string mutexName, bool showRingLockinfo, bool showBufferLockInfo);
#endif
    RingBufferA_context(unsigned ringBufferCapacity, unsigned ringBufferCount);
    ~RingBufferA_context();
    static void myPutCallBackFun(PutBufferChunkRef, void* userData);
    void myPutCallBackFun1(PutBufferChunkRef ref);



    unsigned char* get(unsigned& limitCapacity, size_t  microSeconds = 0);
    void  put(unsigned filledSize);
    bool  pop(PopBufferChunkRef chunkref, size_t  microSeconds);

    void  flush();
private:
    void  enquebuffer();
    void  printGraph(bool inqueue,  size_t curDoingIndex);
private:
    Monitor<RecMutex> _monitor;
    auto_ptr<BufferPool_context> _pool;
    vector<PutBufferChunkRef> _buffers;
    vector<bool>               _inuse;
    size_t  _bytesfilled;
    size_t _fillBufferIndex;
    size_t _eatBufferIndex;
    size_t _buffersUsed;
    size_t _ringBufferCount;
    size_t _bufCapacity;
    size_t _lastlimitCapacity;

//    size_t _totalFree;
//    size_t _waittingSize;
};

#ifdef _ice_verbose
RingBufferA_context::RingBufferA_context(unsigned ringBufferCapacity, unsigned ringBufferCount, string mutexName, bool showRingLockinfo, bool showBufferLockInfo)
:_monitor(mutexName.c_str(), showRingLockinfo)
{
    mutexName += ">>pool:";
    _pool = auto_ptr<BufferPool_context>(new BufferPool_context(mutexName.c_str(), showBufferLockInfo) );
    _ringBufferCount = ringBufferCount;

    for(unsigned i=0; i<ringBufferCount;++i)
    {
        PutBufferChunkRef ref = _pool->allocateBuffer(ringBufferCapacity);
        //printf("[%d]%x\t",i, (unsigned)ref);
        ref->m_callback = RingBufferA_context::myPutCallBackFun;
        ref->m_userData = this;
        _buffers.push_back(ref);
        _inuse.push_back(false);
    }
    //memset(_inuse, 0, sizeof(_inuse));

    _bytesfilled = 0;
    _fillBufferIndex = 0;
    _eatBufferIndex = 0;
    _buffersUsed = 0;
    _lastlimitCapacity = 0;
}
#endif

RingBufferA_context::RingBufferA_context(unsigned ringBufferCapacity, unsigned ringBufferCount)
{
    _pool = auto_ptr<BufferPool_context>(new BufferPool_context() );
    _ringBufferCount = ringBufferCount;
    _bufCapacity = ringBufferCapacity;
    for(unsigned i=0; i<ringBufferCount;++i)
    {
        PutBufferChunkRef ref = _pool->allocateBuffer(ringBufferCapacity);
        //printf("[%d]%x\t",i, (unsigned)ref);
        ref->m_callback = RingBufferA_context::myPutCallBackFun;
        ref->m_userData = this;
        _buffers.push_back(ref);
        _inuse.push_back(false);
    }
    //memset(_inuse, 0, sizeof(_inuse));

    _bytesfilled = 0;
    _fillBufferIndex = 0;
    _eatBufferIndex = 0;
    _buffersUsed = 0;
    _lastlimitCapacity = 0;
}

RingBufferA_context::~RingBufferA_context()
{

}


void RingBufferA_context::myPutCallBackFun(PutBufferChunkRef ref, void* userData)
{
    RingBufferA_context *THIS = (RingBufferA_context*)userData;
    THIS->myPutCallBackFun1(ref);
}


void RingBufferA_context::myPutCallBackFun1(PutBufferChunkRef ref)
{
    Monitor<RecMutex>::Lock lock(_monitor);
    size_t i;
    for(i=0; i<_buffers.size(); ++i)
    {
        if(_buffers[i] == ref)
        {
            if(false == _inuse[i] )
            {
                printf("err !!!! ,  reset false  innuse[%lu]\n", i);
            }
            _inuse[i] = false;
            //cout << "set[" << i << "]=" << (_inuse[i]==1 ? "true" :"false" ) << endl;
            break;
        }
    }




    //printf("put back index:%d\t address:%x, _eatBufferIndex:%d \n",  i, (unsigned)ref->m_data, _eatBufferIndex);

    //cout << "put back index:" <<  i << "\t address:" << ref->m_data << " \t_eatBufferIndex:" << _eatBufferIndex << endl;
//    if(_eatBufferIndex != i)

    //cout << "_eatBufferIndex="<< _eatBufferIndex << "  != put back index=" << i << " !!!!!  " << endl;
    if(_eatBufferIndex != i)
    {
        cout <<"err  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
        cout << "_eatBufferIndex="<< _eatBufferIndex << "  != put back index=" << i << " !!!!!  " << endl;
    }


    _eatBufferIndex = ++_eatBufferIndex%_ringBufferCount;

    printGraph(false, (_eatBufferIndex+_ringBufferCount-1)%_ringBufferCount);
    _monitor.notify();
}





void RingBufferA_context::enquebuffer()
{
    size_t preFillIndex = _fillBufferIndex;
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        _buffers[_fillBufferIndex]->m_dataSize = _bytesfilled;
        //goto next buffer
        _inuse[_fillBufferIndex] = true;		// set in use flag, audio write waiting for this flag
        //cout << "set[" << _fillBufferIndex << "]=" << (_inuse[ _fillBufferIndex]==1 ? "true" :"false" ) << endl;
        _buffersUsed++;
        _fillBufferIndex = ++_fillBufferIndex%_ringBufferCount;
        _bytesfilled = 0;		// reset bytes filled

    }
    _pool->put(_buffers[preFillIndex]);
    printGraph(true, preFillIndex);

}

unsigned char* RingBufferA_context::get(unsigned& limitCapacity, size_t waitMicroSeconds)
{
    //printf("limitCapacity :%d _buffers[0]->m_capacity :%d\n", limitCapacity, _buffers[0]->m_capacity);
    assert(limitCapacity <= _bufCapacity);
    _lastlimitCapacity = limitCapacity;
    do
    {
        {
            Monitor<RecMutex>::Lock lock(_monitor);
            while(_inuse[_fillBufferIndex])
            {
                if (0 >= waitMicroSeconds) 
                    _monitor.wait();
                else
                {
                    if(!_monitor.timedWait(IceUtil::Time::microSeconds(waitMicroSeconds)) )
                        return 0;                    
                }
            }
                
        }


        if(_buffers[_fillBufferIndex]->m_capacity - _bytesfilled >= limitCapacity)
        {
            //limitCapacity = _buffers[_fillBufferIndex]->m_capacity - _bytesfilled;
            return _buffers[_fillBufferIndex]->m_data + _bytesfilled;
        }
        else
        {
            enquebuffer();
        }

    } while(1);
}



void RingBufferA_context::put(unsigned putBytes)
{
    assert(putBytes <= _lastlimitCapacity);
    size_t currentBufferLack = _buffers[_fillBufferIndex]->m_capacity - _bytesfilled;
    assert(currentBufferLack >= putBytes);

    _bytesfilled +=  putBytes;
    if(currentBufferLack == putBytes)
    {
        enquebuffer();
    }

}


bool  RingBufferA_context::pop(PopBufferChunkRef chunkref, size_t microSeconds)
{
    //cout << "RingBufferA_context::pop\n";
    return _pool->pop(chunkref, microSeconds);
}

void  RingBufferA_context::flush()
{
    _pool->flush();
    _bytesfilled = 0;
    _fillBufferIndex = 0;
    _eatBufferIndex = 0;
    _buffersUsed = 0;
    _lastlimitCapacity = 0;    
}


void RingBufferA_context::printGraph(bool inqueue, size_t curDoingIndex)
{
    Monitor<RecMutex>::Lock lock(_monitor);
    printf("%s\t", (inqueue == true ? ">> >> >>:" : "<< << <<:"));
    char buf[20];
    for (size_t i = 0; i < _ringBufferCount; ++i)
    {
        if(i == _fillBufferIndex && i == _eatBufferIndex)
            sprintf(buf,">%s<", (_inuse[i] ? "*" : "_" ) );
        else if(i == _fillBufferIndex)
            sprintf(buf,">>%s", (_inuse[i] ? "*" : "_" ) );
        else if(i == _eatBufferIndex)
            sprintf(buf,"%s<<", (_inuse[i] ? "*" : "_" ) );
        else
            sprintf(buf," %s ", (_inuse[i] ? "*" : "_" ) );

        if(i == curDoingIndex)
            printf("[%s]\t", buf);
        else
            printf(" %s \t", buf);

    }
    printf("\n");
}


//////////////////////////////////////////////////////////////////////////////////////////
RingBufferA::RingBufferA(unsigned ringBufferCapacity, unsigned ringBufferCount)
{
    _ctx = auto_ptr<RingBufferA_context>(new RingBufferA_context(ringBufferCapacity, ringBufferCount) );
}

#ifdef _ice_verbose
RingBufferA::RingBufferA(unsigned ringBufferCapacity, unsigned ringBufferCount, const char* MutexName, bool showLockinfo, bool showBufferLockInfo)
{
    _ctx = auto_ptr<RingBufferA_context>(new RingBufferA_context(ringBufferCapacity, ringBufferCount, MutexName, showLockinfo, showBufferLockInfo) );
}
#endif


RingBufferA::~RingBufferA()
{

}


unsigned char* RingBufferA::get(unsigned& limitCapacity, size_t waitMicroSeconds)
{
    return _ctx->get(limitCapacity, waitMicroSeconds);
}


void  RingBufferA::put(unsigned filledSize)
{
    _ctx->put(filledSize);
}


bool RingBufferA::pop(PopBufferChunkRef chunkref, size_t  microSeconds)
{
     return _ctx->pop(chunkref, microSeconds);
}

void RingBufferA::flush()
{
    _ctx->flush();
}

void RingBufferA::freeBuffers()
{
    _ctx->_pool->freeAll();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
#include <iostream>
#include <IceUtil/Thread.h>
#include "ReStartableThread.h"
RingBufferA* g_buffer;

class ConsumerThread : public IceUtil::Thread
{
public:
    static void myPopCallBackFun(ChunkInfoRef, void* userData, PopbufState);
    void myPopCallBackFun(ChunkInfoRef ref, PopbufState state);
    virtual void run()
    {
        PopBufferChunk chunk;
        chunk.m_callback = ConsumerThread::myPopCallBackFun;
        chunk.m_userData = this;
        chunk.m_fillDataSize = 2 << 12;
        while(1)
        {
            g_buffer->pop(&chunk, 0);

            //printf("get it\n");
            //usleep(1*1000000);
        }
    }
};
void ConsumerThread::myPopCallBackFun(ChunkInfoRef ref, void* userData, PopbufState state)
{
    ConsumerThread* THIS = (ConsumerThread*)userData;
    THIS->myPopCallBackFun(ref, state);
}

void ConsumerThread::myPopCallBackFun(ChunkInfoRef ref, PopbufState state)
{
    //cout << "pop callback >> size: " << ref->m_dataSize << "\tstate:" << state << endl;
}

////////////////////////////////////////////////////////////////////////////
class ConductThread : public IceUtil::Thread
{
public:
    void run();
};


void ConductThread::run()
{
    //static int currentIndex = 0;
    while(1)
    {
        size_t limit = 2<<12;
        unsigned char* buf = g_buffer->get(limit);
        //cout << "buffer free size: " << limit << endl;
        g_buffer->put(2<<12);
        //usleep(100000);
    }
}
int main(int argc, char* argv[])
{

    g_buffer = new RingBufferA(2<<12, 4);

    ConsumerThread consumer;
    consumer.start();
    ConductThread conduct;
    conduct.start();
    consumer.getThreadControl().join();
    conduct.getThreadControl().join();
    return 0;
}
#endif
