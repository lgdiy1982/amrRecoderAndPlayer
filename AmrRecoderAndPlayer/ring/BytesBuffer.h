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
typedef struct ChunkInfo* ChunkInfoRef;
typedef struct PutBufferChunk* PutBufferChunkRef;
typedef struct PopBufferChunk* PopBufferChunkRef;

typedef void (*PutCallBackFun)(void* userData, const ChunkInfoRef,  bool getTerminated);
typedef void (*PopCallBackFun)(ChunkInfoRef, void* userData);

struct ChunkInfo
{
    unsigned char* _data;
    unsigned       _size;
};

struct PutBufferChunk : public ChunkInfo
{
    PutCallBackFun    _callback;
    void*          _userData;
};

struct PopBufferChunk : public ChunkInfo
{
    PopCallBackFun   m_callback;
    void*          m_userData;
};



class BytesBuffer
{
public:
    BytesBuffer(size_t bufferSize);
    size_t put(unsigned char* data, size_t size, size_t timeout = 0);
    unsigned char* pop(size_t size, size_t timeout = 0);
    
private:
    std::auto_ptr<class BytesBuffer_context> _ctx;
};

#endif /* defined(__AmrRecoderAndPlayer__BytesBuffer__) */
