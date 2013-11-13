//
//  AmrFileRecoder.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "AmrFileRecoder.h"
#include "audio/AudioInputUnit.h"
#include <SP.h>
#include <HexDump.h>
//////////////////////////////
#include <CAXException.h>
#import <AudioToolbox/AudioToolbox.h>


@interface AmrFileRecoder()
{
    RecordListener _listener;
    float          _sampleMeter;
    float          _peakSamperMeter;
    
    float          _currentUpdatingMeter;
    float          _currentPeakMeter;
    
    NSUInteger     _currentSliceCount;
    
}

@end



static AmrFileRecoder* instance = nil;
static void progress(void* userData, double acumulateDuration);
static void finished(void* userData, double duration);
static void updateMeters(void* userData, float average, size_t channel);
static void updatePeakMeter(void* userData, float peakPower, size_t channel);

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
        _listener.userData = (__bridge void*)self;
        _listener.progress = progress;
        _listener.finish = finished;
        _listener.updateMeter = updateMeters;
        _listener.updatePeakMeter = updatePeakMeter;
        AudioInputUnit::instance().setRecordListener(_listener);
    }
    return self;
}

- (void) setRecordSession
{
    try {
        UInt32 audioCategory = kAudioSessionCategory_RecordAudio;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(audioCategory), &audioCategory), "couldn't set audio category for playback");
        
        
        Float32 preferredBufferSize = .002;
        XThrowIfError(AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, sizeof(preferredBufferSize), &preferredBufferSize), "couldn't set i/o buffer duration");
        
        XThrowIfError(AudioSessionSetActive(YES), "couldn't set audio session active\n");
    } catch(CAXException e)  {
        char buf[256];
        fprintf(stderr, "Error: %s (%s)\n", e.mOperation, e.FormatError(buf));
    } catch(...) {
        fprintf(stderr, "An unknown error occurred\n");
    }
}

- (void) deactiveRecordSession
{
    XThrowIfError(AudioSessionSetActive(NO), "couldn't set audio session active\n");
}

- (Boolean) startRecordWithFilePath:(NSString*) filepath
{
    if (AudioInputUnit::instance().isRunning()) {
        return NO;
    }
    
    [self setRecordSession];
    _currentSliceCount = 0;
    _sampleMeter = _peakSamperMeter = 33.f;
    _currentUpdatingMeter = 33.f;
    _currentPeakMeter = 33.f;
    return AudioInputUnit::instance().start([filepath UTF8String] );
}

- (Boolean) stopRecord
{
    Boolean ret = AudioInputUnit::instance().stop();
    if (ret) {
        [self deactiveRecordSession];
    }
    return ret;
}

- (Boolean) cancelRecord
{
    return AudioInputUnit::instance().cancel();
}


- (void) progress:(double) acumulateDuration
{
    if(self.delegate)
    {
        [self.delegate recordProgress: acumulateDuration];
    }
}

- (void) finished:(double) duration
{
    if(self.delegate)
    {
        [self.delegate recordFinished:duration];
    }
}


- (void) updateMeters : (float) average withChannel:(NSUInteger) channel
{
    _currentUpdatingMeter = _currentSliceCount/ (_currentSliceCount + 1) * _currentUpdatingMeter + average / (_currentSliceCount + 1);
    _currentSliceCount++;
    //NSLog(@"_currentUpdatingMeter %.2f", _currentUpdatingMeter);
}

- (void) updatePeakMeter : (float) peakPower withChannel:(NSUInteger) channel
{
    if (peakPower > _currentPeakMeter) {
        _currentPeakMeter = peakPower;
    }
}

- (void)  updateMeters
{
    _sampleMeter = _currentUpdatingMeter;
    _currentSliceCount = 0;
    //for peak
    
    _peakSamperMeter = _currentPeakMeter;
    _currentPeakMeter = 33.f;
}

- (float) averagePowerForChannel:(NSUInteger)channelNumber
{
    printf("\n------ %.5f  db = %f \n ", _sampleMeter, 20*log10(_sampleMeter/32767) );
    return 20*log10(_sampleMeter/32767);
}

- (float) peakPowerForChannel:(NSUInteger) channleNumber
{
    printf("\n------ %.5f  db = %f \n ", _peakSamperMeter, 20*log10(_peakSamperMeter/32767) );
    return 20*log10(_peakSamperMeter/32767);
}

@end

void progress(void* userData, double acumulateDuration)
{
    AmrFileRecoder* This = (__bridge AmrFileRecoder*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This progress:acumulateDuration];
    });
}

void finished(void* userData, double duration)
{
    AmrFileRecoder* This = (__bridge AmrFileRecoder*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This finished:duration];
    });
}

void updateMeters(void* userData, float average, size_t channel)
{
    AmrFileRecoder* This = (__bridge AmrFileRecoder*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This updateMeters:average withChannel:channel];
    });
}

void updatePeakMeter(void* userData, float peakPower, size_t channel)
{
    AmrFileRecoder* This = (__bridge AmrFileRecoder*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This updatePeakMeter:peakPower withChannel:channel];
    });
}

void inflateAmrFile(const char* filepath, size_t limit)
{
    FILE *fp = fopen(filepath, "r+");
    fseek(fp, 0L, SEEK_END);
    long filesize = ftell(fp);
    long dataSize = filesize - 6;
    unsigned char *duplicate = (unsigned char*)malloc(filesize + (4 - (filesize)%4)  );
    fseek(fp, 6, SEEK_SET);
    fread(duplicate, 1, dataSize, fp);
    
    fseek(fp, 0L, SEEK_END);
    while (ftell(fp) <= limit)  fwrite(duplicate, 1, dataSize, fp);
    fclose(fp);
    free(duplicate);
}
