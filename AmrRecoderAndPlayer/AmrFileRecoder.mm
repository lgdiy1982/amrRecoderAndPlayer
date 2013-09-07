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
}

@end



static AmrFileRecoder* instance = nil;
static void progress(void* userData, double acumulateDuration);
static void finished(void* userData, double duration);

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




void inflateAmrFile(const char* filepath, size_t limit)
{
    unsigned char replicate[32];
    FILE *fp = fopen(filepath, "r+");
    fseek(fp, 0L, SEEK_END);
    long filesize = ftell(fp);
    SP::printf("filesize %ld", filesize);
    fseek(fp, 6, SEEK_SET);
    fread(replicate, 1, 32, fp);
    bytes2HexS(replicate, 32);
    
//    rewind(fp);
    fseek(fp, 0L, SEEK_END);
    printf("-----------%ld\n",ftell(fp));
    long size = 0;
    while (size < limit) {
        fwrite(replicate, 1, 32, fp);
        size+=32;
        fseek(fp, 32, SEEK_CUR);                                    
        //SP::printf("-----------%ld\n",ftell(fp));
    }
    printf("-----------%ld\n",ftell(fp));
    fclose(fp);
    
//    fp = fopen(filepath, "r");
//    for (int i = 0; i<  20; ++i) {
//        fread(replicate, 1, 32, fp);
//        bytes2HexS(replicate, 32);
//    }
//    fclose(fp);
}
