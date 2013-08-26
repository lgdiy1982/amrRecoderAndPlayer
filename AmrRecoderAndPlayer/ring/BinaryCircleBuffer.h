//
//  BinaryCircleBuffer.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/26/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#ifndef __AmrRecoderAndPlayer__BinaryCircleBuffer__
#define __AmrRecoderAndPlayer__BinaryCircleBuffer__

#include <iostream>
#include <memory>
typedef struct ChunkInfo* ChunkInfoRef;
typedef struct PutBufferChunk* PutBufferChunkRef;
typedef struct PopBufferChunk* PopBufferChunkRef;

typedef void (*PutCallBackFun)(const PutBufferChunkRef, void* userData);
typedef void (*PopCallBackFun)(ChunkInfoRef, void* userData);

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



class BinaryCircleBuffer
{
public:
    BinaryCircleBuffer(size_t bufferSize);
    size_t put(unsigned char* data, size_t size, size_t timeout = 0);
    unsigned char* pop(size_t size, size_t timeout = 0);
    
private:
    std::auto_ptr<class BinaryCircleBuffer_context> _ctx;
};

#endif /* defined(__AmrRecoderAndPlayer__BinaryCircleBuffer__) */
