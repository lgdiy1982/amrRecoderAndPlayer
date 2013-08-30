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


//////////////////////////////



@interface AmrFileRecoder()
{
//    EncodeThreadPtr _encoderThread;
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
        AudioInputUnit::instance().start([_filepath UTF8String] );
//    _encoderThread = new EncodeThread( [_filepath UTF8String]) ;
//    _encoderThread->start();
}

- (void) startRecodeWithFilePath:(NSString*) filepath
{
    _filepath = filepath;
    [self startRecode];
}

- (void) stopRecode
{
    if (AudioInputUnit::instance().isRunning()) {
        AudioInputUnit::instance().stop();
//        AudioInputUnit::instance().flush();
//        AudioInputUnit::instance().uninitialize();
    }
//    _encoderThread->getThreadControl().join();
}

- (void) dealloc
{
    [super dealloc];
}
@end


