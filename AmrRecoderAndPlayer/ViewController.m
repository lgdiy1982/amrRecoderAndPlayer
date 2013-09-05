//
//  ViewController.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "ViewController.h"
#import "AmrFileRecoder.h"
#import "AmrFilePlayer.h"
#import "AmrFileRecoder.h"
@interface ViewController () <UITableViewDataSource, UITableViewDelegate, PlaybackDelegate, RecodeDelegate>
@property (weak, nonatomic) IBOutlet UIButton *playButton;
- (IBAction)play:(id)sender;
- (IBAction)stopPlayback:(id)sender;
@property (weak, nonatomic) IBOutlet UIButton *recodeButton;
- (IBAction)startRecord:(id)sender;
- (IBAction)stopRecord:(id)sender;
- (IBAction)cancelRecord:(id)sender;




@property (weak, nonatomic) IBOutlet UITableView *filesTabelView;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    ((AmrFilePlayer*)[AmrFilePlayer sharedInstance]).delegate = self;
    ((AmrFileRecoder*)[AmrFileRecoder sharedInstance]).delegate = self;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)play:(id)sender {
    NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test.amr"] ;
//    NSString * path = [[NSBundle mainBundle] pathForResource: @"raw_amr.amr" ofType: nil];
//    NSLog(@"%@", path);
    [[AmrFilePlayer sharedInstance] startPlayWithFilePath:path];
}



- (IBAction)stopPlayback:(id)sender {
    [[AmrFilePlayer sharedInstance]  stopPlayback];
}


- (IBAction)startRecord:(id)sender {
    NSString *fileName = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test.amr"] ;
    NSLog(@"%@", fileName);
    [[AmrFileRecoder sharedInstance] startRecordWithFilePath:fileName];
}

- (IBAction)stopRecord:(id)sender {
//    [self.recodeButton setTitle:@"开始" forState:UIControlStateNormal];
   [[AmrFileRecoder sharedInstance] stopRecord];
}

- (IBAction)cancelRecord:(id)sender {
    [[AmrFileRecoder sharedInstance] cancelRecord];
}

#pragma -mark tableview delegate

#pragma -mark tableview datasource


- (void)viewDidUnload {
    
    [super viewDidUnload];
}

- (void) playbackProgress:(double) expired
{
//    NSLog(@"playbackProgress %f %f", expired, duration);
}

- (void) playbackFinished
{
//    NSLog(@"playbackFinished");
}


- (void) recordProgress:(double) acumulateDuration
{
//    NSLog(@"recordProgress %f", acumulateDuration);
}

- (void) recordFinished:(double) duration
{
    NSLog(@"recordFinished");
}


@end
