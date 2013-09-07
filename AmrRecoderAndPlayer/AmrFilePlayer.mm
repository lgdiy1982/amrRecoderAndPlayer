//
//  AmrPlayer.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "AmrFilePlayer.h"
#include "audio/AudioPlayUnit.h"
#import "ASIHTTPRequest.h"
#import "ASIProgressDelegate.h"
enum PlayStatus {
    downloadOnly,
    downlaodThenPlay,
    
};


@interface RequestContext : NSObject
@property (nonatomic) enum PlayStatus _state;
@property (nonatomic) ASIHTTPRequest *request;
@end

@implementation RequestContext

@synthesize _state, request;

@end


@interface AmrFilePlayer()
{
    NSString* _filepath;
    PlaybackListener _listener;
    Boolean _stopped;
    NSMutableDictionary *_fileMapping;
    Boolean _downloading;
    NSMutableDictionary *_requestQueue;
    RequestContext *_currentRequestCtx;
    NSString       *_currentPlayingURL;
}



@end

static void progress(void* userData, double expired);
static void finished(void* userData);


static AmrFilePlayer* instance;
@implementation AmrFilePlayer

+ (id) sharedInstance{
    if (instance == nil) {
        instance = [[AmrFilePlayer alloc] init];
        
    }
    return instance;
}

- (id) init
{
    if( (self = [super init ]) != nil) {
        _fileMapping = [NSMutableDictionary new];
        _requestQueue = [NSMutableDictionary new];
        _downloading = false;
        _currentRequestCtx = nil;
        _currentPlayingURL = nil;
    }
    return self;
}

- (Boolean) startPlayWithFilePath : (NSString*) filepath
{
    _listener.userData = (__bridge void*)self;
    _listener.progress = progress;
    _listener.finish = finished;
    AudioPlayUnit::instance().setPlaybackListener(_listener);
    _filepath = filepath;
    return AudioPlayUnit::instance().startPlay([_filepath UTF8String] );
}

- (Boolean) stopPlayback
{
    return AudioPlayUnit::instance().stopPlay();
}

- (void) stopAll
{
    if (_currentRequestCtx) {
        _currentRequestCtx._state = downloadOnly;       
    }
    else
    {
        AudioPlayUnit::instance().stopPlay();
    }
}

//

- (void) startPlayWithUrl:(NSString* ) url progressDelegate:(ASIProgressDelegate *) downloadDelegate
{
    if ([self isRunning] && [_currentPlayingURL isEqualToString:url]) {
        return;
    }

    [self stopAll];
    NSString* path = [_fileMapping valueForKey:url];
    if (path)   //the file exist
    {
        [self startPlayWithFilePath:path];
        _currentPlayingURL = url;
        _currentRequestCtx = nil;
        if (self.delegate != nil) {
            [self.delegate playbackStart];
        }
    }
    else
    {   
        RequestContext *ctx = [_requestQueue objectForKey:url];
        if (ctx) //still running
        {
            if (ctx._state == downloadOnly) {
                ctx._state = downlaodThenPlay;
                _currentRequestCtx = ctx;
            }
        }
        else    //record the request
        {
            RequestContext *ctx = [[RequestContext alloc] init];

            ASIHTTPRequest * request = [[ASIHTTPRequest alloc] initWithURL:[NSURL URLWithString:url] ];
            [request setDownloadDestinationPath:[
                                                 [NSHomeDirectory() stringByAppendingPathComponent:@"Documents"]
                                                 stringByAppendingPathComponent:
                                                    [url  stringByReplacingOccurrencesOfString:@"/" withString:@"_"]] ];
            [request setDelegate:self];
            [request setDidFinishSelector:@selector(fileFetchComplete:)];
            [request setDidFailSelector:@selector(fileFetchFailed:)];
            [request setDownloadProgressDelegate:downloadDelegate];
            ctx.request = request;
            ctx._state = downlaodThenPlay;
            [_requestQueue setObject:ctx forKey:url];
            _currentRequestCtx = ctx;
            [request startAsynchronous];
        }        
    }
}


- (Boolean) isRunning
{
    return  AudioPlayUnit::instance().isRunning();
}


- (void) progress:(double) expired
{
    if (self.delegate) {
        [self.delegate playbackProgress:expired];
    }
}

- (void) finished
{
    if (self.delegate) {
        [self.delegate playbackFinished];
    }
}

- (void) fileFetchComplete:(ASIHTTPRequest *) request
{
    RequestContext *ctx = [_requestQueue objectForKey: [request.url absoluteString]];
    NSString *fileName = [[request.url absoluteString] stringByReplacingOccurrencesOfString:@"/" withString:@"_"] ;
    [_fileMapping setObject:
                        [[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"] stringByAppendingPathComponent:fileName]
                        forKey: [request.url absoluteString]];
    if (ctx._state == downlaodThenPlay) {
        [self startPlayWithFilePath:[_fileMapping objectForKey:[request.url absoluteString]] ];
        _currentRequestCtx = nil;
        _currentPlayingURL = [request.url absoluteString];
        if (self.delegate != nil) {
            [self.delegate playbackStart];
        }
    }
    [_requestQueue removeObjectForKey:request.url];
}

- (void) fileFetchFailed: (ASIHTTPRequest*) request
{
    RequestContext *ctx = [_requestQueue objectForKey: request.url];
    if (ctx == _currentRequestCtx) {
        _currentRequestCtx = nil;
    }
    NSString *fileName = [[request.url absoluteString] stringByReplacingOccurrencesOfString:@"/" withString:@"_"] ;
    NSString *filepath = [[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"] stringByAppendingPathComponent:fileName];
    NSError* err=nil;
    [[NSFileManager defaultManager] removeItemAtPath:filepath error:&err];
    [_requestQueue removeObjectForKey:request.url];
}



@end

#pragma mark -playback callback
void progress(void* userData, double expired)
{
    AmrFilePlayer* This = (__bridge AmrFilePlayer*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This progress:expired];
    });
}

void finished(void* userData)
{
    AmrFilePlayer* This = (__bridge AmrFilePlayer*)userData;
    dispatch_async(dispatch_get_main_queue(), ^{
        [This finished];
    });
}

int ParseAmrFileDuration(NSString * url)
{
    return parseAmrFileDuration([url UTF8String]);
}


#pragma mark -- asihttprequest delegate

