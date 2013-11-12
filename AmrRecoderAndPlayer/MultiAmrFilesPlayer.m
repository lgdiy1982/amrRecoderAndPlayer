//
//  MultiAmrFilesPlayer.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 9/9/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "MultiAmrFilesPlayer.h"
#import "AmrFilePlayer.h"


#pragma -mark

enum PlayStatus
{
    downloadOnly,
    downlaodThenPlay,
};

@interface RequestContext : NSObject
@property (nonatomic) enum PlayStatus _state;
//@property (nonatomic) ASIHTTPRequest *_request;
@end

@implementation RequestContext
@synthesize _state;//, _request;
@end

static MultiAmrFilesPlayer* instance;

@interface MultiAmrFilesPlayer() <PlaybackDelegate, FetchFileProgress>
{
    NSMutableDictionary *_requestQueueStates;
    RequestContext      *_currentRequestCtx;
    FileFetcher         *_fetcher;
    NSMutableDictionary *_delegates;
    NSURL               *_currentRunningURL;
}
@end


@implementation MultiAmrFilesPlayer
+ (id) sharedInstance
{
    if (instance == nil) {
        instance = [[MultiAmrFilesPlayer alloc] init];
    }
    ((AmrFilePlayer*)[AmrFilePlayer sharedInstance]).delegate = instance;
    return instance;
}

- (id) init
{
    if( (self = [super init ]) != nil) {
        _requestQueueStates = [NSMutableDictionary new];
        _delegates = [NSMutableDictionary new];
        _currentRequestCtx = nil;
        _currentRunningURL = nil;
        ((AmrFilePlayer*)[AmrFilePlayer sharedInstance]).delegate = self;
        
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
        NSString *cachePath = [paths objectAtIndex:0];
        _fetcher = [[FileFetcher alloc] initWithRoot:cachePath MemoryCapacity:0 DiskCapacity:0];
    }
    return self;
}

- (void) startPlayWithURL:(NSURL* ) url
{
    [self stopAll];
    RequestContext *ctx = [[RequestContext alloc] init];
    ctx._state = downlaodThenPlay;
    [_requestQueueStates setObject:ctx forKey:url];
    _currentRequestCtx = ctx;
    [_fetcher fetchFile2Disk:url stateDelegate:self];
    
}

- (Boolean) isRunning:(NSURL*) url
{
    return [_currentRunningURL isEqual:url];
}

- (void) stopAll
{
    for (NSURL *url in [_requestQueueStates allKeys]) {
        if (((RequestContext*)[_requestQueueStates objectForKey:url])._state == downlaodThenPlay) {
            ((RequestContext*)[_requestQueueStates objectForKey:url])._state = downloadOnly;
        }
    }
    [[AmrFilePlayer sharedInstance] stopPlayback];
}


- (NSURL*) getMappedUrl:(NSString*) path
{
    return [_fetcher mappedUrl:path];
}

- (void) startPlayWithURL:(NSURL *)url PlaybackDelegate:(id<SignlePlaybackDelegate>) delegate
{
    [_delegates setObject:delegate forKey:url];
    [self startPlayWithURL:url];
}

#pragma -mark playback callback
- (void) playbackStart : (NSString*) path
{
    _currentRunningURL = [self getMappedUrl:path];
    
    if (self.delegate) {
        [self.delegate multiPlaybackStart:[self getMappedUrl:path] ];
        //NSLog(@"play back start %@", [self getMappedUrl:path]);
    }
    
    id<SignlePlaybackDelegate> delegate = [_delegates objectForKey:[self getMappedUrl:path]];
    if (delegate) [delegate SignlePlaybackStart];
}

- (void) playbackProgress:(NSString*) path Expired: (double) expired;
{
    if (self.delegate) {
        [self.delegate multiPlaybackProgress:[self getMappedUrl:path] Expired:expired];
    }
    
    id<SignlePlaybackDelegate> delegate = [_delegates objectForKey:[self getMappedUrl:path]];
    if (delegate) [delegate SignlePlaybackProgress:expired];
    
}

- (void) playbackFinished : (NSString*) path;
{
    _currentRunningURL = nil;
    if (self.delegate) {
        [self.delegate multiPlaybackFinished:[self getMappedUrl:path]];
        //NSLog(@"play back stop %@", [self getMappedUrl:path]);
    }
    
    
    id<SignlePlaybackDelegate> delegate = [_delegates objectForKey:[self getMappedUrl:path]];
    if (delegate) [delegate SignlePlaybackFinished];
    [_delegates removeObjectForKey:[self getMappedUrl:path]];
    
    [_requestQueueStates removeObjectForKey:[self getMappedUrl:path]];
}

- (void) cacheLocalFile:(NSURL *)url FilePath:(NSString*) filepath
{
    [_fetcher cacheLocalFile:url FilePath:filepath];
}


#pragma -mark fetcher delegate


- (void) fetchProgress: (NSURL*) url Progress:(float) newProgress
{
    
}

- (void) fetchFileComplete:(NSURL*) url LocalPath:(NSString*) localFilepath
{
    RequestContext *ctx = [_requestQueueStates objectForKey: url];
    if (ctx._state == downlaodThenPlay) {
        [[AmrFilePlayer sharedInstance] startPlayWithFilePath:localFilepath ];
    }
}

- (void) fetchFileFailed:(NSURL*) url
{
    [_requestQueueStates removeObjectForKey:url];
}

@end
