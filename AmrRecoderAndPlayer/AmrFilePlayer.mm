//
//  AmrPlayer.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "AmrFilePlayer.h"
#include "audio/AudioPlayUnit.h"
#import "ASIHTTPRequest.h"
#import "ASIProgressDelegate.h"



@interface AmrFilePlayer()
{
    NSString* _filepath;
    PlaybackListener _listener;
}



@end

static void progress(void* userData, double expired);
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
        _filepath = nil;
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
    Boolean ret = AudioPlayUnit::instance().startPlay([_filepath UTF8String] );
    if (self.delegate && ret ==  YES) {
        [self.delegate playbackStart:_filepath];
    }
    return ret;
}

- (Boolean) stopPlayback
{
    Boolean ret = AudioPlayUnit::instance().stopPlay();
    if (self.delegate && ret == YES) {
        [self.delegate playbackFinished:_filepath];
    }
    return ret;
}


- (Boolean) isRunning
{
    return  AudioPlayUnit::instance().isRunning();
}


- (void) progress:(double) expired
{
    if (self.delegate) {
        [self.delegate playbackProgress:_filepath Expired: expired];
    }
}

- (void) finished
{
    if (self.delegate) {
        [self.delegate playbackFinished:_filepath];
    }
}
@end

#pragma mark -playback callback
void progress(void* userData, double expired)
{
    AmrFilePlayer* This = (__bridge AmrFilePlayer*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This progress:expired];
    });
}

void finished(void* userData)
{
    AmrFilePlayer* This = (__bridge AmrFilePlayer*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This finished];
    });
}


#pragma - ultility

int ParseAmrFileDuration(NSString * url)
{
    return parseAmrFileDuration([url UTF8String]);
}




