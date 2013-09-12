//
//  FileFetcher.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 9/8/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "FileFetcher.h"
#import "ASIFormDataRequest.h"
#import <CommonCrypto/CommonDigest.h>
#import "NSString+MD5.h"




#pragma -mark FileEntry
@interface FileEntry (ReferenceCountting)
- (void) retainCount;
- (void) releaseCount;
@end


@interface FileEntry ()
{
    FileFetcher *_fetcher;
    
}
@end


@implementation FileEntry

- (id) initWithFetcher:(FileFetcher*) fetcher
{
    if ( (self = [super init]) != nil) {
        _fetcher = fetcher;
    }
    return self;
}

- (void) dealloc
{
    [self releaseCount];
}

@end

#pragma -mark FileFetcher
@interface FileFetcher() <ASIProgressDelegate>
{
    NSMutableDictionary * _memCache; //<url, >
    NSString *_rootPath;
    NSString *_loadingPath;
    NSString *_readyPath;
    NSUInteger _memCapacity;
    NSUInteger _diskCapacity;
    NSMutableDictionary *_md5result;
    NSMutableDictionary *_delegates;
}
@end

@implementation FileFetcher
- (id) initWithRoot:(NSString*) rootPath  MemoryCapacity:(NSUInteger) memCapcity DiskCapacity:(NSUInteger) diskCapacity
{
    if ((self = [super init]) != nil) {
        _memCapacity = memCapcity;
        _diskCapacity = diskCapacity;
        _md5result = [NSMutableDictionary new];
        _rootPath = rootPath;
        _loadingPath = [NSString stringWithFormat:@"%@/loading/",rootPath] ;
        _readyPath = [NSString stringWithFormat:@"%@/ready/",rootPath] ;
        
        _delegates = [NSMutableDictionary new];
        if (![[NSFileManager defaultManager] fileExistsAtPath:_loadingPath]) {
            [[NSFileManager defaultManager] createDirectoryAtPath:_loadingPath withIntermediateDirectories:YES attributes:nil error:nil];
        }
        if (![[NSFileManager defaultManager] fileExistsAtPath:_readyPath]) {
            [[NSFileManager defaultManager] createDirectoryAtPath:_readyPath withIntermediateDirectories:YES attributes:nil error:nil];
        }
    }
    return  self;
}

- (NSString*) md5:(NSURL*) url
{
    NSString* urlmd5 = [_md5result objectForKey:url];
    if (urlmd5 == nil) {
        urlmd5 = [[url absoluteString] MD5];
        [_md5result setObject:urlmd5 forKey:url];
    }
    return urlmd5;
}

- (void) fetchFile2Disk:(NSURL *)url stateDelegate:(id<FetchFileProgress> ) delegate
{
    NSString * filename = [self md5:url];
    NSString *filepath = [_readyPath stringByAppendingString:filename];
    
    [_md5result setObject:filename forKey:url];
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:filepath]) {
        if (delegate) {
            [delegate fetchFileComplete:url LocalPath:filepath];
            return;
        }
    }
    
    
    NSString *loadingFilename = [_loadingPath stringByAppendingString:filename];
    [_delegates setObject:delegate forKey:url];
    ASIHTTPRequest * request = [[ASIHTTPRequest alloc] initWithURL:url];
    [request setDownloadDestinationPath:loadingFilename];
    [request setDelegate:self];
    //[request setDownloadProgressDelegate:downloadDelegate];
    [request setDidFinishSelector:@selector(fileFetchComplete:)];
    [request setDidFailSelector:@selector(fileFetchFailed:)];
    [request startAsynchronous];
    request = nil;
}

- (void) trimIfNeeded
{
    
}

- (NSString*) getRootPath
{
    return _rootPath;
}

- (NSString*) localPath:(NSURL*) url
{
    return [_readyPath stringByAppendingString:[self md5:url]];
}

- (NSURL*)    mappedUrl:(NSString*) localpath
{
    NSString *filename = [localpath lastPathComponent];
    NSUInteger index = [[_md5result allValues] indexOfObject:filename];
    if (index == NSNotFound) {
        return nil;
    }
    return [[_md5result allKeys] objectAtIndex:index];
}

- (void) fileFetchComplete:(ASIHTTPRequest *) request
{
    NSString *filename = [self md5:request.url];
    NSString *loadingFilename = [_loadingPath stringByAppendingString:filename];
	NSString* readyFilename = [_readyPath stringByAppendingString:filename];
	NSError* err=nil;
	[[NSFileManager defaultManager] moveItemAtPath:loadingFilename toPath:readyFilename error:&err];
    if (err) {
		NSLog(@"move file %@",err);
		exit(0);
	}
    
    id<FetchFileProgress> delegate = [_delegates objectForKey:request.url];
    
    if (delegate) {
        [delegate fetchFileComplete:request.url LocalPath: [_readyPath stringByAppendingString:filename] ];
        [_delegates removeObjectForKey:request.url];
    }
}


- (void) fileFetchFailed: (ASIHTTPRequest*) request
{
    NSString *filename = [[request.url absoluteString] MD5];
    NSString *loadingFilename = [_loadingPath stringByAppendingString:filename];
    NSError* err=nil;
    [[NSFileManager	defaultManager] removeItemAtPath:loadingFilename error:&err];
    if (err) {
		NSLog(@"remove file  %@",err);
		exit(0);
	}
    
    id<FetchFileProgress> delegate = [_delegates objectForKey:request.url];
    if (delegate) {
        [delegate fetchFileFailed:request.url];
        [_delegates removeObjectForKey:request.url];
    }
}

- (void) cacheLocalFile:(NSURL *)url FilePath:(NSString*) filepath
{
    NSString *filename = [self md5:url];
    NSString* readyFilename = [_readyPath stringByAppendingString:filename];
    NSError* err=nil;
    if ([[NSFileManager defaultManager] fileExistsAtPath:filepath])
    {
        [[NSFileManager defaultManager] removeItemAtPath:readyFilename error:&err];
        [[NSFileManager	defaultManager] moveItemAtPath:filepath toPath:readyFilename error:&err];
    }
}

@end
