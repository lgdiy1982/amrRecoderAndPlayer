//
//  BinaryCircleBuffer.cpp
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/26/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#include "BytesBuffer.h"
#include <IceUtil/IceUtil.h>
using namespace IceUtil;
typedef  unsigned char* byte_;

class BytesBuffer_context
{
public:
    BytesBuffer_context(size_t bufferSize);
    ~BytesBuffer_context();
    friend class BytesBuffer;
    
    
    void put(size_t size, PutBufferChunkRef cbChunk);
    void get(size_t size, PopBufferChunkRef cbChunk);
    
    
    unsigned char* borrow(size_t);
    void giveBack(unsigned char*);
    
    void terminatePut();
    void terminateGet();
private:
    const  size_t _totalBufferSize;
    size_t _totalFreeSize;
    size_t _feedBeginIndex;
    size_t _feedCapacity;
    
    size_t _eatBeginIndex;
    size_t _eatCapacity;
    Monitor<RecMutex> _monitor;
    unsigned char* _buffer;
    size_t _magnification;
    size_t _divisor;
    
    PutBufferChunkRef _putCB;
    bool _putTerminated;
    bool _getTerminated;
};

BytesBuffer_context::BytesBuffer_context(size_t bufferSize) :
_totalBufferSize(bufferSize),
_feedBeginIndex(0),
_feedCapacity(bufferSize),
_eatBeginIndex(0),
_eatCapacity(0),
_putTerminated(false),
_getTerminated(false)
{
    _buffer = (unsigned char*)malloc(_totalFreeSize);
}

void BytesBuffer_context::put(size_t size, PutBufferChunkRef cbChunk)
{
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        while( size > _feedCapacity && !_getTerminated) {
            _monitor.wait();
        }
    }
    
    if (_getTerminated) {
        cbChunk->_data = NULL;
        cbChunk->_size = 0;
        cbChunk->_callback(cbChunk->_userData, cbChunk, true);
        return;
    }
    
    bool truncated = false;
    size_t truncatedSize = 0;

    //at this moment , _feedCapacity maybe increased, but do not infect the result;
    if (_totalBufferSize < _feedBeginIndex + _feedCapacity) {
        truncated = true;
        truncatedSize = _totalBufferSize - _feedBeginIndex;
        
    }
    
    if (truncated) {        
        cbChunk->_data = (unsigned char*)malloc(size);
        cbChunk->_size = size;
        //give outsize the continus buffer
        cbChunk->_callback(cbChunk->_userData, cbChunk, false);
        //refill
        memcpy(_buffer+_feedBeginIndex, cbChunk->_data, truncatedSize);
        memcpy(_buffer, cbChunk->_data, size-truncatedSize);
    }
    else {
        cbChunk->_data = _buffer+_feedBeginIndex;
        cbChunk->_size = size;
        cbChunk->_callback(cbChunk->_userData, cbChunk, false);
    }
    
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        _eatCapacity += size;
        _feedBeginIndex = (_feedBeginIndex + size)%_totalBufferSize;
        _feedCapacity -= size;
        //check and notify
        _monitor.notify();
    }
}

void BytesBuffer_context::get(size_t size, PopBufferChunkRef cbChunk)
{
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        while(size > _eatCapacity && !_putTerminated) {
            _monitor.wait();
        }
    }
    
    if (_putTerminated) {
        cbChunk->_data = NULL;
        cbChunk->_size = 0;
        cbChunk->_callback(cbChunk->_userData, cbChunk, true);
        return;
    }
    
    
}


void BytesBuffer_context::terminatePut()
{
    Monitor<RecMutex>::Lock lock(_monitor);
    _putTerminated = true;
}

void BytesBuffer_context::terminateGet()
{
    Monitor<RecMutex>::Lock lock(_monitor);
    _getTerminated = true;
}

//void BytesBuffer_context::get(PopBufferChunkRef chunkref, size_t  microSeconds);
//{
//    
//}



BytesBuffer_context::~BytesBuffer_context()
{
    delete _buffer;
}

//////////////////////////////////////////////////////
//
//BytesBuffer::BytesBuffer(size_t magnification, size_t divisor)
//{
//    _ctx = std::auto_ptr<BytesBuffer_context>(new BytesBuffer_context(magnification, divisor) );
//}


//void BytesBuffer::put(unsigned char* data, size_t size, size_t timeout)
//{
//    
//    {
//        Monitor<RecMutex>::Lock  lock(_ctx->_monitor);
//        while () {
//            
//        }
//    }
//
//}


