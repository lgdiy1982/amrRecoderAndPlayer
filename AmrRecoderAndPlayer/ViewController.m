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

#import "ASIFormDataRequest.h"



@interface ViewController () <UITableViewDataSource, UITableViewDelegate, PlaybackDelegate, RecodeDelegate>
{
    ASIFormDataRequest *request;
    IBOutletCollection(UIButton) NSArray *amrPlayerButtons;
    IBOutletCollection(UIProgressView) NSArray *amrDownloadProgressbars;
    NSArray *_urls;
    NSInteger _curIndex;
}

@property (weak, nonatomic) IBOutlet UIButton *playButton;
- (IBAction)play:(id)sender;
- (IBAction)stopPlayback:(id)sender;
@property (weak, nonatomic) IBOutlet UIButton *recodeButton;
- (IBAction)startRecord:(id)sender;
- (IBAction)stopRecord:(id)sender;
- (IBAction)cancelRecord:(id)sender;
- (IBAction)togglePlay:(id)sender;




@property (weak, nonatomic) IBOutlet UITableView *filesTabelView;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    ((AmrFilePlayer*)[AmrFilePlayer sharedInstance]).delegate = self;
    ((AmrFileRecoder*)[AmrFileRecoder sharedInstance]).delegate = self;
    _urls = [NSArray arrayWithObjects:
             @"http://192.168.0.105/539b5eccd21f1d342476dad63e406964",
             @"http://192.168.0.105/6a4f5248b4f3f2cf6fd1d2e7059962ab",
             @"http://192.168.0.105/9c1cd9e6642e7ab0fd154546ecf5c324",
             @"http://192.168.0.105/bbd3cbe17a49355d3b5c850944f88977",
             @"http://192.168.0.105/8d093acab1b70ba97cf2b43c13143912",
             nil];
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

- (IBAction)togglePlay:(id)sender {
    NSUInteger index = [amrPlayerButtons indexOfObject:sender];

    if ([@"play" isEqualToString:((UIButton*)sender).titleLabel.text ]) {
        [[AmrFilePlayer sharedInstance] startPlayWithUrl:[_urls objectAtIndex:index] progressDelegate: [amrDownloadProgressbars objectAtIndex:index]];
    }
    else if ([@"stop" isEqualToString:((UIButton*)sender).titleLabel.text]) {
        [[AmrFilePlayer sharedInstance] stopAll];
    }
}

#pragma -mark tableview delegate

#pragma -mark tableview datasource


- (void)viewDidUnload {
    
    amrPlayerButtons = nil;
    amrDownloadProgressbars = nil;
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
//    NSLog(@"recordFinished");
    NSString *fileName = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test.amr"] ;
    inflateAmrFile( [fileName UTF8String], 1<<23);
    [self upload];
}


- (void) upload
{
    request = [ASIFormDataRequest requestWithURL:[NSURL URLWithString:@"http://192.168.0.105/index.php"] ];
//  [request setPostValue:@"test" forKey:@"value1"];
//	[request setPostValue:@"test" forKey:@"value2"];
//	[request setPostValue:@"test" forKey:@"value3"];
    request.delegate = self;
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_4_0
	[request setShouldContinueWhenAppEntersBackground:YES];
#endif
//	[request setUploadProgressDelegate:progressIndicator];
	[request setDelegate:self];
	[request setDidFailSelector:@selector(uploadFailed:)];
	[request setDidFinishSelector:@selector(uploadFinished:)]
    ;
    
    NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test.amr"] ;
    [request setFile:path forKey:[NSString stringWithFormat:@"test"]];
    [request startAsynchronous];
}


- (void) uploadFailed:(ASIHTTPRequest *)theRequest
{
    NSLog(@"uploadFailed");
}

- (void) uploadFinished:(ASIHTTPRequest *)theRequest
{
    NSLog(@"%@", [theRequest responseString]);
    //theRequest
}

- (void) download
{
    
}

- (void) playbackStart
{
    
}
//- ()
@end
