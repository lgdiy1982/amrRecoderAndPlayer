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
- (void) playbackProgress:(double) expired ;
- (void) playbackFinished;
@end

#ifdef __cplusplus
extern "C"
#endif
int ParseAmrFileDuration(NSString * url);

@interface AmrFilePlayer : NSObject
@property (assign, nonatomic) id<PlaybackDelegate> delegate;


+ (id) sharedInstance;
- (Boolean) startPlayWithFilePath : (NSString*) filepath;
- (Boolean) stopPlayback;
- (Boolean) isRunning;
@end
