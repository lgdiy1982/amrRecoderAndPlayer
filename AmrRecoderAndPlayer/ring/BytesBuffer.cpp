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
    const size_t _totalBufferBytes;
    size_t _totalFreeSize;
    size_t _feedBeginIndex;
    size_t _feedEndIndex;
    size_t _eatBeginIndex;
    size_t _eatEndIndex;
    Monitor<RecMutex> _monitor;
    unsigned char* _buffer;
    size_t _magnification;
    size_t _divisor;
    
    PutBufferChunkRef _putCB;
    bool _putTerminated;
    bool _getTerminated;
};

BytesBuffer_context::BytesBuffer_context(size_t bufferSize) :
_totalBufferBytes(bufferSize),
_totalFreeSize(bufferSize),
_feedBeginIndex(0),
_feedEndIndex(0),
_eatBeginIndex(0),
_eatEndIndex(0),
_putTerminated(false),
_getTerminated(false)
{
    _buffer = (unsigned char*)malloc(_totalFreeSize);
}

void BytesBuffer_context::put(size_t size, PutBufferChunkRef cbChunk)
{
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        while(size > _totalFreeSize && !_getTerminated) {
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
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        if (_feedBeginIndex + size > _totalBufferBytes) {
            truncated = true;
            truncatedSize = _totalBufferBytes - _feedBeginIndex;
            
        } else {
            _eatEndIndex += size;
        }
    }
    
    if (truncated) {        
        cbChunk->_data = (unsigned char*)malloc(size);
        cbChunk->_size = size;
        cbChunk->_callback(cbChunk->_userData, cbChunk, false);
        memcpy(_buffer+_feedBeginIndex, cbChunk->_data, truncatedSize);
        memcpy(_buffer, cbChunk->_data, size-truncatedSize);
        free(cbChunk->_data);

    }
    else {
        cbChunk->_data = _buffer+_feedBeginIndex;
        cbChunk->_size = size;
        cbChunk->_callback(cbChunk->_userData, cbChunk, false);
    }
}

void BytesBuffer_context::get(size_t size, PopBufferChunkRef cbChunk)
{
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        while(size > _totalFreeSize && !_getTerminated) {
            _monitor.wait();
        }
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


