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
#include <CAXException.h>
#import <AudioToolbox/AudioToolbox.h>



@interface AmrFilePlayer()
{
    NSString* _filepath;
    PlaybackListener _listener;
    Boolean  _reachEnd;
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
        _listener.userData = (__bridge void*)self;
        _listener.progress = progress;
        _listener.finish = finished;
        AudioPlayUnit::instance().setPlaybackListener(_listener);
        
        _reachEnd = NO;
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(sensorStateChange:)
                                                     name:UIDeviceProximityStateDidChangeNotification
                                                   object:nil];
    }
    return self;
}


- (void) sessionDeactivity
{
    try {
        XThrowIfError(AudioSessionSetActive(NO), "couldn't set audio session deactive\n");
    } catch(CAXException e)  {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    } catch(...) {
        fprintf(stderr, "An unknown error occurred\n");
    }
}

- (void) setSpeakerSession
{
    try {
        UInt32 audioCategory = kAudioSessionCategory_MediaPlayback;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory), "couldn't set audio category for PlayAndRecord");
        Float32 preferredBufferSize = .01;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");
        XThrowIfError(AudioSessionSetActive(YES), "couldn't set audio session active\n");
    } catch(CAXException e)  {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    } catch(...) {
        fprintf(stderr, "An unknown error occurred\n");
    }
}

- (void) setReciverSession
{
    try {
        UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory), "couldn't set audio category for PlayAndRecord");
        
        UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;
        XThrowIfError(AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute, sizeof (audioRouteOverride), &audioRouteOverride), "couldn't set AudioRoute to reciver") ;
        
        Float32 preferredBufferSize = .01;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");
        XThrowIfError(AudioSessionSetActive(YES), "couldn't set audio session active\n");
    } catch(CAXException e)  {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    } catch(...) {
        fprintf(stderr, "An unknown error occurred\n");
    }
}

- (void) alter2SpeakerSession
{
    AudioPlayUnit::instance().pausePlay();
    [self setSpeakerSession];
    AudioPlayUnit::instance().resume();
}

- (void) alter2ReciverSession
{
    AudioPlayUnit::instance().pausePlay();
    [self setReciverSession];
    AudioPlayUnit::instance().resume();
}

- (void) sessionUnInit
{
    AudioSessionSetActive(NO);
}


- (Boolean) startPlayWithFilePath : (NSString*) filepath
{
    if (AudioPlayUnit::instance().isRunning()) {
        return NO;
    }
    [self setSpeakerSession];
    _filepath = filepath;
    Boolean ret = AudioPlayUnit::instance().startPlay([_filepath UTF8String] );
    if (ret) {
        NSLog(@"startPlayWithFilePath  setProximityMonitoringEnabled:YES");
        [[UIDevice currentDevice] setProximityMonitoringEnabled:YES];
        _reachEnd = NO;
        if (self.delegate) {
            [self.delegate playbackStart:_filepath];
        }
    }
    
    return ret;
}

- (Boolean) stopPlayback
{
    if (!AudioPlayUnit::instance().isRunning()) {
        return NO;
    }
    
    Boolean ret = AudioPlayUnit::instance().stopPlay();
    if (ret == YES) {
        [self sessionDeactivity];
         NSLog(@"stopPlayback  setProximityMonitoringEnabled:NO");
        [[UIDevice currentDevice] setProximityMonitoringEnabled:NO];
        if (self.delegate) {
            [self.delegate playbackFinished:_filepath];
        }
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
    [self sessionDeactivity];
    _reachEnd = YES;
    if (self.delegate) {
        [self.delegate playbackFinished:_filepath];
    }
}


-(void) sensorStateChange:(NSNotificationCenter *)notification
{
    NSLog(@"proximity == %@" , [[UIDevice currentDevice] proximityState] ? @"YES" : @"NO");
    
    if ([[UIDevice currentDevice] proximityState] == YES)
    {
        [self alter2ReciverSession];
    }
    else
    {
        if (_reachEnd) {
            NSLog(@"onfinished   setProximityMonitoringEnabled:NO");
            [[UIDevice currentDevice] setProximityMonitoringEnabled:NO];
        }
        else
        {
            [self alter2SpeakerSession];
        }
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




