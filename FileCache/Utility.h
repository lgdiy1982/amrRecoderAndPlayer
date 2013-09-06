//
//  Utility.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 9/6/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface Utility : NSObject
+ (id) sharedInstance;
- (id) initWithRootPath:(NSString*)root;
- (NSString*) getFilePath:(NSString*) key;


//@property (assign, nonatomic) NSInteger capacity;


@end

@interface FileFetcher : NSObject
- (void) remove:(NSString*) url;
- (NSString*) mapPath:(NSString*) url;


@end


