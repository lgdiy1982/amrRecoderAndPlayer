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
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(sensorStateChange:)
                                                     name:UIDeviceProximityStateDidChangeNotification
                                                   object:nil];
        [self sessionInit];
    }
    return self;
}


- (void) sessionInit
{
    try {
        UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory), "couldn't set audio category for playback");

        
        Float32 preferredBufferSize = .002;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");
        
        UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
        XThrowIfError(AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute, sizeof (audioRouteOverride), &audioRouteOverride), "couldn't set AudioRoute to speaker") ;
//        Float64 hwSampleRate;
//        UInt32 size = sizeof(hwSampleRate);
//        XThrowIfError(AudioSessionGetProperty(kAudioSessionProperty_CurrentHardwareSampleRate, &size, &hwSampleRate), "couldn't get hw sample rate");
        
        XThrowIfError(AudioSessionSetActive(true), "couldn't set audio session active\n");
    } catch(CAXException e)  {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    } catch(...) {
        
    }
}

- (void) sessionUnInit
{
    AudioSessionSetActive(NO);
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
        [[UIDevice currentDevice] setProximityMonitoringEnabled:YES];
        [self.delegate playbackStart:_filepath];
    }
    return ret;
}

- (Boolean) stopPlayback
{
    Boolean ret = AudioPlayUnit::instance().stopPlay();
    if (self.delegate && ret == YES) {
        [[UIDevice currentDevice] setProximityMonitoringEnabled:NO];
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
        UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
        XThrowIfError(AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute, sizeof (audioRouteOverride), &audioRouteOverride), "couldn't set AudioRoute to speaker") ;
        [[UIDevice currentDevice] setProximityMonitoringEnabled:NO];
        [self.delegate playbackFinished:_filepath];
    }
}


-(void) sensorStateChange:(NSNotificationCenter *)notification
{
    if ([[UIDevice currentDevice] proximityState] == YES)
    {
        NSLog(@"proximity == YES");
        //uninit audio unit
        AudioPlayUnit::instance().pausePlay();
        
        //
        
        try {
            XThrowIfError(AudioSessionSetActive(NO), "couldn't set audio session deactive\n");
            
            UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord;
            XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory), "couldn't set audio category for PlayAndRecord");
            
            UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;
            XThrowIfError(AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute, sizeof (audioRouteOverride), &audioRouteOverride), "couldn't set AudioRoute to reciver") ;
            
            
            XThrowIfError(AudioSessionSetActive(YES), "couldn't set audio session active\n");
        } catch(CAXException e)  {
            char buf[256];
            fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
        } catch(...) {
            fprintf(stderr, "An unknown error occurred\n");
        }
        
        AudioPlayUnit::instance().resume();
    }
    else
    {
        UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
        XThrowIfError(AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute, sizeof (audioRouteOverride), &audioRouteOverride), "couldn't set AudioRoute to speaker") ;
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




