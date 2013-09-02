//
//  BytesBuffer.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/26/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#ifndef __AmrRecoderAndPlayer__BytesBuffer__
#define __AmrRecoderAndPlayer__BytesBuffer__

#include <iostream>
#include <memory>
#include <IceUtil/Shared.h>
#include <IceUtil/Handle.h>
typedef struct ChunkInfo* ChunkInfoRef;
typedef struct BufferChunk* BufferChunkRef;

typedef size_t (*CallBackFun)(void* userData, const ChunkInfoRef,  bool terminated);

struct ChunkInfo
{
    unsigned char* _data;
    size_t       _size;
};

struct BufferChunk : public ChunkInfo
{
    CallBackFun    _callback;
    void*          _userData;
};




class BytesBuffer : public IceUtil::Shared
{
public:
    BytesBuffer(size_t bufferSize);
    void feed(size_t size, BufferChunkRef cbChunk);
    void eat(size_t size, BufferChunkRef cbChunk);
    //call this two terminated method to reset the status, notice, the oppsite termiate method must be called after done the last feed/eat action. usually in out of the thread runloop
    void terminatedFeed();
    void terminatedEat();
    bool empty();
private:
    std::auto_ptr<class BytesBuffer_context> _ctx;
};

typedef IceUtil::Handle<BytesBuffer> BytesBufferPtr;
#endif /* defined(__AmrRecoderAndPlayer__BytesBuffer__) */
