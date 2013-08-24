//
//  AmrPlayer.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol playerDelegate
@optional
- (CGFloat) timeEscaped;
@end

@interface AmrFilePlayer : NSObject
@property (nonatomic) id<playerDelegate> id;
@property (nonatomic) NSURL* filepath;
- (void) start : (NSURL*) filepath;
- (void) start;
- (void) stop;
@end
