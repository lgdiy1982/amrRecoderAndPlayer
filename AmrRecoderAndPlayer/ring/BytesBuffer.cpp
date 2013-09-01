//
//  BinaryCircleBuffer.cpp
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/26/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#include "BytesBuffer.h"
#include <IceUtil/IceUtil.h>
#include <SP.h>
#include <HexDump.h>
using namespace IceUtil;




class BytesBuffer_context
{
public:
    BytesBuffer_context(size_t bufferSize);
    ~BytesBuffer_context();
    friend class BytesBuffer;
    
    
    void feed(size_t size, BufferChunkRef cbChunk);
    void eat(size_t size, BufferChunkRef cbChunk);
    void clean();
    void terminateFeed();
    void terminateEat();
    bool empty();
private:
    const  size_t _totalBufferSize;
    size_t _feedBeginIndex;
    size_t _feedCapacity;
    
    size_t _eatBeginIndex;
    size_t _eatCapacity;
    Monitor<RecMutex> _monitor;
    unsigned char* _buffer;
    
    bool _feedTerminated;
    bool _eatTerminated;
};

BytesBuffer_context::BytesBuffer_context(size_t bufferSize) :
_totalBufferSize(bufferSize),
_feedBeginIndex(0),
_feedCapacity(bufferSize),
_eatBeginIndex(0),
_eatCapacity(0),
_feedTerminated(false),
_eatTerminated(false)
{
    _buffer = (unsigned char*)malloc(bufferSize);
    bzero(_buffer, bufferSize);
}

void BytesBuffer_context::feed(size_t size, BufferChunkRef cbChunk)
{
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        if (_feedTerminated) return;

        
        while( size > _feedCapacity && !_eatTerminated) {
            _monitor.wait();
        }
    }
    
    if (_eatTerminated) {
        clean();
        return;
    }

    bool truncated = false;
    size_t truncatedSize = 0;

    //at this moment , _feedCapacity maybe increased, but do not disturb the result;
    if (_totalBufferSize < _feedBeginIndex + size) {
        truncated = true;
        truncatedSize = _totalBufferSize - _feedBeginIndex;
    }
    
    if (truncated) {        
        cbChunk->_data = (unsigned char*)malloc(size);
        cbChunk->_size = size;
        //give out  the continious buffer
        size = cbChunk->_callback(cbChunk->_userData, cbChunk, false);
        //refill
        if (size <= truncatedSize) {
            memcpy(_buffer+_feedBeginIndex, cbChunk->_data, size);
        }        
        else  {
            memcpy(_buffer+_feedBeginIndex, cbChunk->_data, truncatedSize);
            memcpy(_buffer, cbChunk->_data+truncatedSize, size-truncatedSize);
        }
        free(cbChunk->_data);
    }
    else
    {
        cbChunk->_data = _buffer+_feedBeginIndex;
        cbChunk->_size = size;
        size = cbChunk->_callback(cbChunk->_userData, cbChunk, false);
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

void BytesBuffer_context::eat(size_t size, BufferChunkRef cbChunk)
{
    //bool canEat = false;
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        while(size > _eatCapacity && !_feedTerminated) {            
            _monitor.wait();
        }
        //canEat = (_eatCapacity >= size);
    }

    //terminated
    size = size > _eatCapacity ? _eatCapacity : size;

    
    
    bool truncated = false;
    size_t truncatedSize = 0;
    //at this moment , _eatCapacity maybe increased, but do not disturb the result;
    if (_totalBufferSize < _eatBeginIndex + size ) {
        truncated = true;
        truncatedSize = _totalBufferSize - _eatBeginIndex ;
    }
    
    if (truncated) {
        cbChunk->_data = (unsigned char*)malloc(size);
        cbChunk->_size = size;
        //SP::printf("eat size %u\n", size);
        memcpy(cbChunk->_data, _buffer+_eatBeginIndex, truncatedSize);
        memcpy(cbChunk->_data + truncatedSize, _buffer, size-truncatedSize);
        size = cbChunk->_callback(cbChunk->_userData, cbChunk, _feedTerminated);
        free(cbChunk->_data);
        //g_printer->printf("eat truncatedSize %d", truncatedSize);
    }
    else
    {
        cbChunk->_data = _buffer+_eatBeginIndex;
        cbChunk->_size = size;
        //SP::printf("eat size %u\n", size);
        size = cbChunk->_callback(cbChunk->_userData, cbChunk, _feedTerminated);
    }

    
    {
        Monitor<RecMutex>::Lock lock(_monitor);
        _eatCapacity -= size;
        _eatBeginIndex = (_eatBeginIndex + size)%_totalBufferSize;
        _feedCapacity += size;
    }

    
    if (_feedTerminated && _eatCapacity == 0) //nomore
    {
        cbChunk->_data = 0;
        cbChunk->_size = 0;
        
        cbChunk->_callback(cbChunk->_userData, cbChunk, _feedTerminated);
        clean();
    }
    else if (!_feedTerminated && _eatCapacity > 0) {
        Monitor<RecMutex>::Lock lock(_monitor);
        _monitor.notify();
    }
    else if(_eatCapacity > 0 && _feedTerminated)
    {
        //continue
    }
    

}

void BytesBuffer_context::clean()
{
    _feedBeginIndex = 0;
    _feedCapacity = _totalBufferSize;
    
    _eatBeginIndex = 0;
    _eatCapacity = 0;

    _feedTerminated = false;
    _eatTerminated = false;
}

void BytesBuffer_context::terminateFeed()
{
    Monitor<RecMutex>::Lock lock(_monitor);
    if(_feedTerminated != true)
    {
        _feedTerminated = true;
        _monitor.notify();
    }
}

void BytesBuffer_context::terminateEat()
{
    Monitor<RecMutex>::Lock lock(_monitor);
    if(_eatTerminated != true )
    {
        _eatTerminated = true;
        _monitor.notify();
    }
}

bool BytesBuffer_context::empty()
{
    Monitor<RecMutex>::Lock lock(_monitor);
    return _feedCapacity == _totalBufferSize;
}

BytesBuffer_context::~BytesBuffer_context()
{
    free(_buffer);
}



//--------------------------------------------------------------------------------------------
BytesBuffer::BytesBuffer(size_t bufferSize)
{
    _ctx = std::auto_ptr<BytesBuffer_context>(new BytesBuffer_context(bufferSize));
}


void BytesBuffer::feed(size_t size, BufferChunkRef cbChunk)
{
    _ctx->feed(size, cbChunk);
}


void BytesBuffer::eat(size_t size, BufferChunkRef cbChunk)
{
    _ctx->eat(size, cbChunk);
}


void BytesBuffer::terminatedFeed()
{
    _ctx->terminateFeed();
}


void BytesBuffer::terminatedEat()
{
    _ctx->terminateEat();
}

bool BytesBuffer::empty()
{
    return  _ctx->empty();
}
