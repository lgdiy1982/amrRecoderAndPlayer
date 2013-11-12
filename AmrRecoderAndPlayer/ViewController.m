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
#import "FileFetcher.h"
#import "MultiAmrFilesPlayer.h"
#import "POVoiceHUD.h"
@interface ViewController () <PlaybackDelegate, RecodeDelegate, MultiPlaybackDelegate>
{
    ASIFormDataRequest *request;
    IBOutletCollection(UIButton) NSArray *amrPlayerButtons;
    IBOutletCollection(UIProgressView) NSArray *amrDownloadProgressbars;
    NSArray *_urls;
    NSInteger _curIndex;
    FileFetcher *_fetcher;
    
    POVoiceHUD *_voiceHud;
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
    
    ((MultiAmrFilesPlayer*)[MultiAmrFilesPlayer sharedInstance]).delegate = self;
    _urls = [NSArray arrayWithObjects:
             @"http://192.168.0.105/b40323a90283ac05e9f1cf1bb06cf8e3",
             @"http://192.168.0.105/1d386476fe79b13e55210b3b26c2c0ee",
             @"http://192.168.0.105/e144fe0e81c4214de2e46bb80976ab6a",
             @"http://192.168.0.105/ebe7527c1e72d4b25af0c13739ac6a42",
             @"http://192.168.0.105/4f07845466c1eaf0adce0ff6347ae380",

             nil];
    
//    _fetcher = [[FileFetcher alloc] initWithRoot:NSTemporaryDirectory() MemoryCapacity:0 DiskCapacity:0];
}

- (void) viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    _voiceHud = [[POVoiceHUD alloc] initWithParentView:self.view];
    _voiceHud.title = @"Speak Now";
    [self.view addSubview:_voiceHud];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)play:(id)sender {
    NSString *path = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test.amr"] ;
     ((AmrFilePlayer*)[AmrFilePlayer sharedInstance]).delegate = self;
    if ( [self.playButton.titleLabel.text isEqualToString: @"play"] ) {
        [[AmrFilePlayer sharedInstance] startPlayWithFilePath:path];
        [((UIButton*)sender) setTitle:@"stop" forState: UIControlStateNormal];
    } else {
        [[AmrFilePlayer sharedInstance]  stopPlayback];
        [self.playButton setTitle:@"play" forState: UIControlStateNormal];
    }
    
}



- (IBAction)stopPlayback:(id)sender {
    [[AmrFilePlayer sharedInstance]  stopPlayback];
}


- (IBAction)startRecord:(id)sender {
    NSString *fileName = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test.amr"] ;
    [_voiceHud startForFilePath:fileName];

}

- (IBAction)stopRecord:(id)sender {
   [_voiceHud stopRecording];
}

- (IBAction)cancelRecord:(id)sender {
    [_voiceHud cancelRecording];
}

- (IBAction)togglePlay:(id)sender {
    NSUInteger index = [amrPlayerButtons indexOfObject:sender];
    if ([@"play" isEqualToString:((UIButton*)sender).titleLabel.text ]) {
        [[MultiAmrFilesPlayer sharedInstance] startPlayWithURL: [NSURL URLWithString: [_urls objectAtIndex:index]] ];
    }
    else if ([@"stop" isEqualToString:((UIButton*)sender).titleLabel.text]) {
        [[MultiAmrFilesPlayer sharedInstance] stopAll];
    }
}



- (void)viewDidUnload {
    
    amrPlayerButtons = nil;
    amrDownloadProgressbars = nil;
    [super viewDidUnload];
}


#pragma -mark record delegate

- (void) recordProgress:(double) acumulateDuration
{
//    NSLog(@"recordProgress %f", acumulateDuration);
}

- (void) recordFinished:(double) duration
{
//    NSLog(@"recordFinished");
//    NSString *fileName = [NSTemporaryDirectory() stringByAppendingPathComponent: @"test.amr"] ;
//    inflateAmrFile( [fileName UTF8String], 1<<22);
//    [self upload];
}


- (void) upload
{
    request = [ASIFormDataRequest requestWithURL:[NSURL URLWithString:@"http://192.168.0.105/index.php"] ];
    request.delegate = self;
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_4_0
	[request setShouldContinueWhenAppEntersBackground:YES];
#endif
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

#pragma -mark playback delegate

- (void) playbackStart:(NSString*) path
{
    
}
- (void) playbackProgress:(NSString*) path Expired:(double) expired
{
    
}
- (void) playbackFinished:(NSString*) path
{
    [self.playButton setTitle:@"play" forState: UIControlStateNormal];
}
//- ()

#pragma -mark mult player delegate
- (void) multiPlaybackStart:(NSURL*) url
{
    NSUInteger index = [_urls indexOfObject:[url absoluteString]];
    [[amrPlayerButtons objectAtIndex:index] setTitle:@"stop" forState:UIControlStateNormal];
}

- (void) multiPlaybackProgress:(NSURL*) url Expired:(double) expired
{
    
}

- (void) multiPlaybackFinished:(NSURL*) url
{
    NSUInteger index = [_urls indexOfObject:[url absoluteString]];
    [[amrPlayerButtons objectAtIndex:index] setTitle:@"play" forState:UIControlStateNormal];
}
@end
