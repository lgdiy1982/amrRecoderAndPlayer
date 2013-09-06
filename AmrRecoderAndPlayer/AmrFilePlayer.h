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
- (void) playbackStart;
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

//if playback is running, you should stop first, twice playback on same url is noeffect
- (void) startPlayWithUrl:(NSString* ) url;
- (void) stopAll;
- (Boolean) stopPlayback;
- (Boolean) isRunning;
@end


/*
@protocol RemotePlaybackDelegate
- (void) playbackStart;
- (void) playbackProgress:(double) expired;
- (void) playbackFishished;
@end

@interface RemoteAMRFilePlayer : NSObject
+ (id) sharedInstance;
- (void) startPlayWithUrl:(NSString* ) url;
- (void) stopPlayback;
@end
*/