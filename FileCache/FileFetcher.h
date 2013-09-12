//
//  FileFetcher.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 9/8/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "ASIHTTPRequest.h"
#import "ASIProgressDelegate.h"

enum CachePolicy
{
    EDiskCache = 0x01,
    EMemoryCache = 0x02,
    EAutomatic = 0x04,
    EManully = 0x08,
    EMeoryAndDisk = EDiskCache | EMemoryCache,
};


@interface FileEntry : NSObject
@property (nonatomic) NSURL* url;
@property (nonatomic) NSString *path;
@end

@protocol FetchFileProgress <NSObject>
- (void) fetchProgress: (NSURL*) url Progress:(float) newProgress;
- (void) fetchFileComplete:(NSURL*) url LocalPath:(NSString*) localFilepath;
- (void) fetchFileFailed:(NSURL*) url;
@end


@interface FileFetcher : NSObject
//- (void) initWithRoot:(NSString*) rootPath MemoryCapacity:(NSUInteger) memCapcity DiskCapacity:(NSUInteger) diskCapacity CachePolicy:(enum CachePolicy) policy;
- (NSString*) getRootPath;
- (NSString*) localPath:(NSURL*) url;
- (NSURL*)    mappedUrl:(NSString*) localpath;

//test has memory cache
- (id) initWithRoot:(NSString*) rootPath  MemoryCapacity:(NSUInteger) memCapcity DiskCapacity:(NSUInteger) diskCapacity;
- (void) fetchFile2Disk:(NSURL *)url stateDelegate:(id<FetchFileProgress> ) delegate;
- (void) cacheLocalFile:(NSURL *)url FilePath:(NSString*) filepath;

//just disk cache
@end
