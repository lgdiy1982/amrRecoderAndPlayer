//
//  AmrPlayer.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol PlaybackDelegate
@optional
- (void) playbackProgress:(double) expired totalDuration:(double) duration;
- (void) playbackFinished;
@end

@interface AmrFilePlayer : NSObject
@property (weak, nonatomic) id<PlaybackDelegate> delegate;


+ (id) sharedInstance;
- (Boolean) startPlayWithFilePath : (NSString*) filepath;
- (Boolean) stopPlayback;

@end
