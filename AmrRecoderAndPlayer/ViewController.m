//
//  ViewController.m
//  AmrRecoderAndPlayer
//
//  Created by lu gang on 8/24/13.
//  Copyright (c) 2013 topcmm. All rights reserved.
//

#import "ViewController.h"
#import "AmrFileRecoder.h"
@interface ViewController () <UITableViewDataSource, UITableViewDelegate>
@property (weak, nonatomic) IBOutlet UIButton *playButton;
- (IBAction)playOrStop:(id)sender;

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
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)playOrStop:(id)sender {
    
}


- (IBAction)startRecord:(id)sender {
//    [self.recodeButton setTitle:@"停止" forState:UIControlStateNormal];
    [[AmrFileRecoder sharedInstance] startRecodeWithFilePath:@""];
}

- (IBAction)stopRecord:(id)sender {
//    [self.recodeButton setTitle:@"开始" forState:UIControlStateNormal];
//    [[AmrFileRecoder sharedInstance] stopRecode];
}

- (IBAction)cancelRecord:(id)sender {
    
}

#pragma -mark tableview delegate

#pragma -mark tableview datasource


@end
