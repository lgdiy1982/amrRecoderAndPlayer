//
//  AmrFileRecoder.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol RecodeDelegate
@optional
- (void) recordProgress:(double) acumulateDuration;;
- (void) recordFinished;
- (void) updateMeter:(double) meter;
@end

@interface AmrFileRecoder : NSObject
@property (assign, nonatomic) id<RecodeDelegate> delegate;

+ (id) sharedInstance;
- (Boolean) startRecordWithFilePath:(NSString*) filepath;
- (Boolean) stopRecord;
- (Boolean) cancelRecord;
@end
