//
//  AmrFileRecoder.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "AmrFileRecoder.h"
#include "audio/AudioInputUnit.h"
#include "ring/ReStartableThread.h"
#include <memory>
#include <string>
#include <interf_enc.h>
#include <iostream>


#define AMR_MAGIC_NUMBER "#!AMR\n"

#define PCM_FRAME_SIZE 160 // 8khz 8000*0.02=160
#define MAX_AMR_FRAME_SIZE 32
#define AMR_FRAME_COUNT_PER_SECOND 50

class EncodeThread : public IceUtil::Thread
{
public:
    EncodeThread(const std::string& filepath);
    void closeFile();
    virtual void run();
    virtual ~EncodeThread();
//    friend void myPopCallBackFun(ChunkInfoRef, void* userData, PopbufState state, unsigned fillneedSize);
private:
//    PopBufferChunk  chunk;
    std::string _filepath;
    void * armEncodeState;
};

typedef IceUtil::Handle<EncodeThread> EncodeThreadPtr;

//void myPopCallBackFun(ChunkInfoRef ref, void* userData, PopbufState state, unsigned fillneedSize)
//{
//    EncodeThread *thiz = (EncodeThread*)userData;
//    //encode
//    enum Mode req_mode;
//    req_mode = MR122;
//    short input[PCM_FRAME_SIZE];
//    unsigned char output[MAX_AMR_FRAME_SIZE];
//
//    if (e_whole == state) {
//        
//    }
//    
//    int ret = Encoder_Interface_Encode(thiz->armEncodeState, req_mode, input, output, 0);
//    std::cout << (size_t)state << "   " << ref->m_dataSize << std::endl;
//}

EncodeThread::EncodeThread(const std::string& filepath) :_filepath(filepath)
{
//    chunk.m_callback = myPopCallBackFun;
//    chunk.m_userData = this;
//    chunk.m_fillDataSize = 160;
}

void EncodeThread::run()
{
    
    int dtx = 0;
    armEncodeState = Encoder_Interface_init(dtx);
    while (1) {
//        AudioInputUnit::instance().getData(&chunk, 0.1*1000000);
    }
    Encoder_Interface_exit(&armEncodeState);
}

EncodeThread::~EncodeThread()
{
    
}

//////////////////////////////



@interface AmrFileRecoder()
{
    EncodeThreadPtr _encoderThread;
    NSString* _filepath;
}

@end



static AmrFileRecoder* instance = nil;
@implementation AmrFileRecoder

+ (id) sharedInstance{
    if (instance == nil) {
        instance = [[AmrFileRecoder alloc] init];
    }
    return instance;
}

- (id) init
{
    if ((self = [super init]) != nil) {

    }
    return self;
}

- (void) startRecode
{
    if (!AudioInputUnit::instance().isInitialized())
        AudioInputUnit::instance().initialize(8000.0, 1, 16);
    if (!AudioInputUnit::instance().isRunning())
        AudioInputUnit::instance().startRec();
    _encoderThread = new EncodeThread( [_filepath UTF8String]) ;
    _encoderThread->start();
}

- (void) startRecodeWithFilePath:(NSString*) filepath
{
    _filepath = filepath;
    [self startRecode];
}

- (void) stopRecode
{
    if (AudioInputUnit::instance().isRunning()) {
        AudioInputUnit::instance().stopRec();
        AudioInputUnit::instance().flush();
        AudioInputUnit::instance().uninitialize();
    }
    _encoderThread->getThreadControl().join();
}

- (void) dealloc
{
    [super dealloc];
}
@end


