//
//  AppDelegate.h
//  video-mush-gui-mac
//
//  Created by Visualisation on 26/02/2014.
//  Copyright (c) 2014. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Automator/Automator.h>

#import <Video Mush/exports.hpp>

#import "liveStreamWindowController.h"
#import "dataOutputWindow.h"
#import "mushPopUpNavigator.h"
#import "parSettings.h"
#import "mushQueue.h"
#import "openglWindowController.h"


@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (assign) IBOutlet NSWindow *window;
@property (weak) IBOutlet NSTextField *exposureValue;
@property (weak) IBOutlet NSTextField *gammaValue;
@property (weak) IBOutlet NSSegmentedControl *compressMethod;
@property (weak) IBOutlet NSTabView *inputMethod;
@property (weak) IBOutlet NSTabView *outputMethod;
@property (weak) IBOutlet NSTabView *processMethod;
@property (weak) IBOutlet NSButton *encodeButton;
@property (weak) IBOutlet NSButton *statsButton;
@property (weak) IBOutlet NSPopUpButton *ffmpegPathButton;
@property (weak) IBOutlet mushPopUpNavigator *exrPathButton;
@property (weak) IBOutlet mushPopUpNavigator *folderPathButton;
@property (weak) IBOutlet NSPopUpButton *videoPathButton;
@property (weak) IBOutlet mushPopUpNavigator *yuv12bitPathButton;
@property (weak) IBOutlet NSMatrix *exposureCount;
@property (weak) IBOutlet NSMatrix *exposureOrder;
@property (weak) IBOutlet NSTextField *firstISO;
@property (weak) IBOutlet NSTextField *secondISO;
@property (weak) IBOutlet NSTextField *thirdISO;
@property (weak) IBOutlet NSTextField *fourthISO;
@property (weak) IBOutlet NSMatrix *cameraSwitch;
- (IBAction)ffmpegPathButton:(id)sender;
- (IBAction)encodeClicked:(id)sender;
- (IBAction)statsClicked:(id)sender;
@property (weak) IBOutlet NSUserDefaultsController *userDefaults;
@property (weak) IBOutlet NSButton *showGUI;
@property (weak) IBOutlet NSButton *sim2Preview;
@property (weak) IBOutlet NSButton *testCard;
@property (weak) IBOutlet NSTextField *x264Path;
@property (unsafe_unretained) IBOutlet NSTextView *statsBox;


@property (weak) IBOutlet NSButton *slicCellBorders;
@property (weak) IBOutlet NSButton *slicCellCenters;
@property (weak) IBOutlet NSButton *slicFillCells;
@property (weak) IBOutlet NSButton *slicUniqueness;
@property (weak) IBOutlet NSButton *slicDistribution;
@property (weak) IBOutlet NSButton *slicSaliency;


@property (weak) IBOutlet NSMatrix *waveformMode;
@property (weak) IBOutlet NSMatrix *flareResolution;
@property (weak) IBOutlet NSTextField *ffmpegFPS;
@property (weak) IBOutlet NSPopUpButton *ffmpegEncoder;
@property (weak) IBOutlet NSButton *framesLoop;
@property (weak) IBOutlet NSPopUpButton *fbdOutput;

- (mush::config)getConfig;
@property (nonatomic, retain) NSTimer * timer;
@property (nonatomic, retain) NSTimer * timer2;
@property (weak) IBOutlet NSButton *yuvUsePQ;
@property (weak) IBOutlet NSTextField *yuvBitDepth;
@property (weak) IBOutlet NSTextField *yuvMaxValue;
@property (weak) IBOutlet NSButton *yuvDryRun;

@property (weak) IBOutlet NSButton *flareSerial;
@property (weak) IBOutlet NSButton *useLogC;
@property (weak) IBOutlet NSButton *useLogCV;
@property (weak) IBOutlet NSButton *pqLegacy;
@property (weak) IBOutlet NSPopUpButton *transferFunc;

@property mush::processEngine processEngine;
@property (weak) IBOutlet NSPopUpButton *ffmpegTransfer;
@property (weak) IBOutlet NSPopUpButton *inputTransfer;
@property (weak) IBOutlet NSPopUpButton *genericChoice;
@property (weak) IBOutlet NSButton *useHLS;
@property (weak) IBOutlet NSButton *demoMode;
- (IBAction)overrideChecked:(id)sender;
@property (weak) IBOutlet NSButton *overrideSize;
@property (weak) IBOutlet NSTextField *overrideWidth;
@property (weak) IBOutlet NSTextField *overrideHeight;
@property (weak) IBOutlet NSTextField *flareSerialPath;
@property (weak) IBOutlet NSTextField *crfBox;

@property (weak) IBOutlet NSButton *dualISO;
@property (weak) IBOutlet NSPopUpButton *whiteBalance;

- (IBAction)clearButton:(id)sender;
- (IBAction)dualISOClicked:(id)sender;
- (IBAction)livestream_checked:(id)sender;
@property (weak) IBOutlet NSButton *livestream_button;
- (IBAction)livestream_button_clicked:(id)sender;

@property (strong, nonatomic) liveStreamWindowController * liveStreamWindow;
@property (strong, nonatomic) dataOutputWindow * dataOutput;

@property (strong, nonatomic) parSettings * parSettingWindow;

@property (strong, nonatomic) mushQueue * mushQueueWindow;

@property (strong, nonatomic) openglWindowController * openglWindow;

- (IBAction)data_button_clicked:(id)sender;

@property (weak) IBOutlet NSButton *resizeCheck;
- (IBAction)resize_checked:(id)sender;
@property (weak) IBOutlet NSTextField *input_resize_width;
@property (weak) IBOutlet NSTextField *input_resize_height;
- (IBAction)resetButtonClicked:(id)sender;
@property (weak) IBOutlet NSButton *catchExceptions;
@property (weak) IBOutlet NSButton *dummyRun;
@property (unsafe_unretained) IBOutlet NSTextView *textOutput;
@property (weak) IBOutlet NSColorWell *bg_colour;
@property (weak) IBOutlet NSColorWell *text_colour;

@property (weak) IBOutlet NSPopUpButton *imageFileType;


@property (weak) IBOutlet NSButton *sponza;
@property (weak) IBOutlet NSButton *sibenik;
@property (weak) IBOutlet NSButton *office;
@property (weak) IBOutlet NSButton *kitchen;
@property (weak) IBOutlet NSTextField *par_width;
@property (weak) IBOutlet NSTextField *par_height;

@property (weak) IBOutlet NSTextField *par_iterations;
@property (weak) IBOutlet NSTextField *par_offset;
@property (weak) IBOutlet NSTextField *par_count;
@property (weak) IBOutlet NSButton *par_opengl;
- (IBAction)sceneRadioSelect:(id)sender;
@property (weak) IBOutlet NSButton *videoLoop;
@property (weak) IBOutlet NSButton *oculusCheck;

@property (weak) IBOutlet NSTextField *amdShadow;
@property (weak) IBOutlet NSTextField *amdAORays;
@property (weak) IBOutlet NSTextField *amdBounces;

@property (weak) IBOutlet NSButton *amdShareCL;
@property (weak) IBOutlet NSButton *amdAOEnable;
@property (weak) IBOutlet NSButton *amdProgressive;

@property (weak) IBOutlet NSTextField *amdSamples;
@property (weak) IBOutlet NSTextField *amdAORadius;

@property (weak) IBOutlet NSPopUpButton *amdCameraType;
@property (weak) IBOutlet NSTextField *amdFocalLength;
@property (weak) IBOutlet NSTextField *amdFocusDistance;
@property (weak) IBOutlet NSTextField *amdAperture;
@property (weak) IBOutlet NSTextField *amdSensorSize;
@property (weak) IBOutlet NSTextField *amdEnvMapMult;

@property (weak) IBOutlet mushPopUpNavigator *singleImagePath;
@property (weak) IBOutlet NSButton *autoDecodeCheck;
@property (weak) IBOutlet NSPopUpButton *parCameraType;

@property (weak) IBOutlet NSButton *lockInputFPS;
@property (weak) IBOutlet NSTextField *lockFPSText;
@property (weak) IBOutlet mushPopUpNavigator *secondStillBox;
@property (weak) IBOutlet mushPopUpNavigator *thirdStillBox;
@property (weak) IBOutlet mushPopUpNavigator *fourthStillBox;
- (IBAction)parSettingsButtonClicked:(id)sender;
@property (weak) IBOutlet NSButton *par360player;
@property (weak) IBOutlet mushPopUpNavigator *secondFolderPath;
@property (weak) IBOutlet mushPopUpNavigator *thirdFolderPath;
@property (weak) IBOutlet mushPopUpNavigator *fourthFolderPath;
@property (weak) IBOutlet mushPopUpNavigator *genericCameraPath;
@property (weak) IBOutlet NSTextField *genericSpeedFactor;
@property (weak) IBOutlet NSButton *genericAutoCamera;
@property (weak) IBOutlet NSPopUpButton *genericCameraType;
- (IBAction)queueButtonClicked:(id)sender;
- (bool)runConfig:(mush::config)config;
@property (weak) IBOutlet mushPopUpNavigator *modelPath;
@property (weak) IBOutlet NSButton *parUseMushModel;
@property (weak) IBOutlet NSTextField *genericModelScale;
@property (weak) IBOutlet NSTextField *generic_camera_x;
@property (weak) IBOutlet NSTextField *generic_camera_y;
@property (weak) IBOutlet NSTextField *generic_camera_z;
@property (weak) IBOutlet NSTextField *generic_camera_theta;
@property (weak) IBOutlet NSTextField *generic_camera_phi;
@property (weak) IBOutlet NSTextField *generic_camera_fov;
@property (weak) IBOutlet NSButton *parFourOutput;
@property (weak) IBOutlet NSButton *parFlipNormals;
@property (weak) IBOutlet NSButton *full_screen;
@property (weak) IBOutlet NSButton *genericStereo;
@property (weak) IBOutlet NSButton *generic_spherical;
@property (weak) IBOutlet mushPopUpNavigator *genericSaveCameraPath;
@property (weak) IBOutlet NSButton *genericQuitAtEnd;
@property (weak) IBOutlet NSButton *reprojStereoPath;
@property (weak) IBOutlet NSButton *reprojStereoToPers;
@property (weak) IBOutlet NSButton *exrTimestamp;
@property (weak) IBOutlet mushPopUpNavigator *secondVideoPath;
- (IBAction)nodeClick:(id)sender;

@end
