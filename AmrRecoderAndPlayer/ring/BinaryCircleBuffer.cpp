//
//  BinaryCircleBuffer.cpp
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/26/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#include "BinaryCircleBuffer.h"
#include <IceUtil/IceUtil.h>
using namespace IceUtil;
typedef  unsigned char* byte_;

class BinaryCircleBuffer_context
{
public:
    BinaryCircleBuffer_context(size_t bufferSize);
    ~BinaryCircleBuffer_context();
    friend class BinaryCircleBuffer;
    
    void put(byte_ *data, size_t size, size_t timeoutMilliseconds);
    Byte* get(size_t size, size_t timeoutMilliseconds, size_t ret);
    void terminatePut();
    void terminateGet();
private:
    const size_t _totalFreeBytes;
    size_t _fillIndex;
    size_t _eatIndex;
    Monitor<RecMutex> _monitor;
    unsigned char* _buffer;
    size_t _magnification;
    size_t _divisor;
};

BinaryCircleBuffer_context::BinaryCircleBuffer_context(size_t bufferSize) : 
_totalFreeBytes(bufferSize)
{
    _buffer = (unsigned char*)malloc(_totalFreeBytes);
}

void BinaryCircleBuffer_context::put(byte_ *data, size_t size, size_t timeoutMilliseconds)
{
    
}

void BinaryCircleBuffer_context::terminatePut()
{
    
}

void BinaryCircleBuffer_context::terminateGet()
{
    
}

//void BinaryCircleBuffer_context::get(PopBufferChunkRef chunkref, size_t  microSeconds);
//{
//    
//}



BinaryCircleBuffer_context::~BinaryCircleBuffer_context()
{
    delete _buffer;
}

//////////////////////////////////////////////////////
//
//BinaryCircleBuffer::BinaryCircleBuffer(size_t magnification, size_t divisor)
//{
//    _ctx = std::auto_ptr<BinaryCircleBuffer_context>(new BinaryCircleBuffer_context(magnification, divisor) );
//}


//void BinaryCircleBuffer::put(unsigned char* data, size_t size, size_t timeout)
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


unsigned char* BinaryCircleBuffer::pop(size_t size, size_t waitMicroSeconds)
{

    {
        Monitor<RecMutex>::Lock lock(_ctx->_monitor);
        while(size > _ctx->_totalFreeBytes) {
            if (0 >= waitMicroSeconds)
                _ctx->_monitor.wait();
            else
            {
                if(!_ctx->_monitor.timedWait(IceUtil::Time::microSeconds(waitMicroSeconds)) )
                    return 0;
            }
        }
    }
    
}