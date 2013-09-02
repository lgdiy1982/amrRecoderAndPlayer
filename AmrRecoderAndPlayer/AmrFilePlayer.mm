//
//  AmrPlayer.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "AmrFilePlayer.h"
#include "audio/AudioPlayUnit.h"


@interface AmrFilePlayer()
{
    NSString* _filepath;
    PlaybackListener _listener;
}



@end

static void progress(void* userData, double expired, double duration);
static void finished(void* userData);


static AmrFilePlayer* instance;
@implementation AmrFilePlayer

+ (id) sharedInstance{
    if (instance == nil) {
        instance = [[AmrFilePlayer alloc] init];
    }
    return instance;
}

- (id) init
{
    if( (self = [super init ]) != nil) {

    }
    return self;
}

- (Boolean) startPlayWithFilePath : (NSString*) filepath
{
    _listener.userData = (__bridge void*)self;
    _listener.progress = progress;
    _listener.finish = finished;
    AudioPlayUnit::instance().setPlaybackListener(_listener);
    _filepath = filepath;
    return AudioPlayUnit::instance().startPlay([_filepath UTF8String] );
}

- (Boolean) stopPlayback
{
     return AudioPlayUnit::instance().stopPlay() ;
}
     
- (void) progress:(double) expired  totalDuration:(double) duration
{
    
    if (self.delegate) {
        [self.delegate playbackProgress:expired totalDuration:duration];
    }
}

- (void) finished
{
    if (self.delegate) {
        [self.delegate playbackFinished];
    }
}
@end


void progress(void* userData, double expired, double duration)
{
    AmrFilePlayer* This = (__bridge AmrFilePlayer*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This progress:expired totalDuration:duration];
    });
}

void finished(void* userData)
{
    AmrFilePlayer* This = (__bridge AmrFilePlayer*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This finished];
    });
}
