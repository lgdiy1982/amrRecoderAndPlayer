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
@interface ViewController () <UITableViewDataSource, UITableViewDelegate, PlaybackDelegate>
@property (weak, nonatomic) IBOutlet UIButton *playButton;
- (IBAction)playOrStop:(id)sender;

@property (weak, nonatomic) IBOutlet UIButton *recodeButton;
- (IBAction)startRecord:(id)sender;
- (IBAction)stopRecord:(id)sender;
- (IBAction)cancelRecord:(id)sender;
- (IBAction)stopPlayback:(id)sender;



@property (weak, nonatomic) IBOutlet UITableView *filesTabelView;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    ((AmrFilePlayer*)[AmrFilePlayer sharedInstance]).delegate = self;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)playOrStop:(id)sender {
//    NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test.amr"] ;
    NSString * path = [[NSBundle mainBundle] pathForResource: @"raw_amr.amr" ofType: nil];
//    NSLog(@"%@", path);
    [[AmrFilePlayer sharedInstance] startPlayWithFilePath:path];
}



- (IBAction)stopPlayback:(id)sender {
    [[AmrFilePlayer sharedInstance]  stopPlayback];
}


- (IBAction)startRecord:(id)sender {
//    [self.recodeButton setTitle:@"停止" forState:UIControlStateNormal];
//    CFUUIDRef uuidObject = CFUUIDCreate(kCFAllocatorDefault);
//    NSString *uuidStr = (__bridge_transfer NSString *)CFUUIDCreateString(kCFAllocatorDefault, uuidObject);
//    CFRelease(uuidObject);
    
    
//    NSArray *paths=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory
//                                                       , NSUserDomainMask
//                                                       , YES);
//    
//    NSString *fileName=[[paths objectAtIndex:0] stringByAppendingPathComponent:uuidStr];

    NSString *fileName = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test.amr"] ;
//    [[NSFileManager defaultManager] changeCurrentDirectoryPath:[paths stringByExpandingTildeInPath]];
//    [[NSFileManager defaultManager] createFileAtPath:uuidStr contents:nil attributes:nil];
//    NSURL *tmpDirURL = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
//    NSURL *fileURL = [[tmpDirURL URLByAppendingPathComponent:@"pkm"] URLByAppendingPathExtension:@"jpg"];
//    NSLog(@"fileURL: %@", [fileURL path]);
    
    NSLog(@"%@", fileName);

    [[AmrFileRecoder sharedInstance] startRecordWithFilePath:fileName];
}

- (IBAction)stopRecord:(id)sender {
//    [self.recodeButton setTitle:@"开始" forState:UIControlStateNormal];
   [[AmrFileRecoder sharedInstance] stopRecord];
}

- (IBAction)cancelRecord:(id)sender {
    
}

#pragma -mark tableview delegate

#pragma -mark tableview datasource


- (void)viewDidUnload {
    
    [super viewDidUnload];
}

- (void) playbackProgress:(double) expired totalDuration:(double) duration
{
    NSLog(@"playbackProgress %f %f", expired, duration);
}

- (void) playbackFinished
{
    NSLog(@"playbackFinished");
}

@end
