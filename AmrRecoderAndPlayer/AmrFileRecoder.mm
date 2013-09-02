//
//  AmrFileRecoder.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "AmrFileRecoder.h"
#include "audio/AudioInputUnit.h"
//////////////////////////////



@interface AmrFileRecoder()
{
    RecordListener _listener;
}

@end



static AmrFileRecoder* instance = nil;
static void progress(void* userData, double acumulateDuration);
static void finished(void* userData);

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
        _listener.userData = self;
        _listener.progress = progress;
        _listener.finish = finished;
        AudioInputUnit::instance().setRecordListener(_listener);
    }
    return self;
}

- (Boolean) startRecordWithFilePath:(NSString*) filepath
{
    
    return AudioInputUnit::instance().start([filepath UTF8String] );
}

- (Boolean) stopRecord
{
    return AudioInputUnit::instance().stop();
}

- (Boolean) cancelRecord
{
    return AudioInputUnit::instance().cancel();
}

- (void) dealloc
{
    [super dealloc];
}

- (void) progress:(double) acumulateDuration
{
    if(self.delegate)
    {
        [self.delegate recordProgress: acumulateDuration];
    }
}

- (void) finished
{
    if(self.delegate)
    {
        [self.delegate recordFinished];
    }
}
@end

void progress(void* userData, double acumulateDuration)
{
    AmrFileRecoder* This = (AmrFileRecoder*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This progress:acumulateDuration];
    });
}

void finished(void* userData)
{
    AmrFileRecoder* This = (AmrFileRecoder*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This finished];
    });
}

