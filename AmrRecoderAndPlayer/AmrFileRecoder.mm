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



@interface AmrFileRecoder()
{
    RecordListener _listener;
    Boolean        _isUpdatingMeter;
    float          _sampleMeter;
    float          _currentUpdatingMeter;
    NSUInteger     _currentSliceCount;
}

@end



static AmrFileRecoder* instance = nil;
static void progress(void* userData, double acumulateDuration);
static void finished(void* userData, double duration);
static void updateMeters(void* userData, float average, size_t channel);

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
        AudioInputUnit::instance().setRecordListener(_listener);
    }
    return self;
}

- (Boolean) startRecordWithFilePath:(NSString*) filepath
{
    _sampleMeter =  -59;
    _currentSliceCount = 0;
    _currentUpdatingMeter = -60.f;
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
    _currentUpdatingMeter = (_currentSliceCount * _currentUpdatingMeter + average) / (_currentSliceCount + 1);
    _currentSliceCount++;
//    if (_currentSliceCount == 5)
//    {
//        _currentSliceCount = 0;
//        _currentUpdatingMeter = 0;
//    }

}


- (void)  updateMeters
{
    _sampleMeter = _currentUpdatingMeter;
    _currentSliceCount = 0;
    _currentUpdatingMeter = -59.f;
}

- (float) averagePowerForChannel:(NSUInteger)channelNumber
{
    printf("------ %.5f", _sampleMeter);
    return 20*log10(_sampleMeter/32767);
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
