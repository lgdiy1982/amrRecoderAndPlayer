//
//  AmrFileRecoder.h
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import <Foundation/Foundation.h>
#ifdef __cplusplus
extern "C" 
#endif
void inflateAmrFile(const char* filepath, size_t limit);


@protocol RecodeDelegate
@optional
- (void) recordProgress:(double) acumulateDuration;;
- (void) recordFinished:(double) duration;
@end

@interface AmrFileRecoder : NSObject
@property (assign, nonatomic) id<RecodeDelegate> delegate;

+ (id) sharedInstance;
- (Boolean) startRecordWithFilePath:(NSString*) filepath;
- (Boolean) stopRecord;
- (Boolean) cancelRecord;

- (void)  updateMeters;
- (float) averagePowerForChannel:(NSUInteger)channelNumber;
- (float) peakPowerForChannel:(NSUInteger) channleNumber;
@end
