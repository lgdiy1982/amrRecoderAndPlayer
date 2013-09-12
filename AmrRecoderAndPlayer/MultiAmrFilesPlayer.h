//
//  MultiAmrFilesPlayer.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 9/9/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "FileFetcher.h"
//@protocol fetchFileProgress;

@protocol MultiPlaybackDelegate
@optional
- (void) multiPlaybackStart:(NSURL*) url;
- (void) multiPlaybackProgress:(NSURL*) url Expired:(double) expired ;
- (void) multiPlaybackFinished:(NSURL*) url;
@end

@protocol SignlePlaybackDelegate <NSObject>
@optional
- (void) SignlePlaybackStart;
- (void) SignlePlaybackProgress:(double) expired;
- (void) SignlePlaybackFinished;
@end


@interface MultiAmrFilesPlayer : NSObject
@property (nonatomic, weak) id<MultiPlaybackDelegate> delegate;
+ (id) sharedInstance;
- (void) startPlayWithURL:(NSURL*) url;
- (void) startPlayWithURL:(NSURL *)url PlaybackDelegate:(id<SignlePlaybackDelegate>) delegate;
- (Boolean) isRunning:(NSURL*) url;
- (void) stopAll;
- (void) cacheLocalFile:(NSURL *)url FilePath:(NSString*) filepath;
@end
