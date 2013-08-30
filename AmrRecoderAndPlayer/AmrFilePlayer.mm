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
}



@end
static AmrFilePlayer* instance;
@implementation AmrFilePlayer

+ (id) sharedInstance{
    if (instance == nil) {
        instance = [[AmrFilePlayer alloc] init];
    }
    return instance;
}
- (void) startPlayWithFilePath : (NSString*) filepath
{
    _filepath = filepath;
    [self startPlay];
}


- (void) startPlay
{
    if (!AudioPlayUnit::instance().isInitialized())
        AudioPlayUnit::instance().initialize(8000.0, 1, 16);
    if (!AudioPlayUnit::instance().isRunning())
        AudioPlayUnit::instance().startPlay([_filepath UTF8String] );
}


- (void) stop
{
    
}
@end
