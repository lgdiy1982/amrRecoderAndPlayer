//
//  AmrPlayer.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import <Foundation/Foundation.h>

@class ASIProgressDelegate;
@protocol PlaybackDelegate
@optional
- (void) playbackStart:(NSString*) path;
- (void) playbackProgress:(NSString*) path Expired:(double) expired ;
- (void) playbackFinished:(NSString*) path;
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



