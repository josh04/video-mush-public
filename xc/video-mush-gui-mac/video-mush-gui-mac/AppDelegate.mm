//
//  AppDelegate.m
//  video-mush-gui-mac
//
//  Created by Visualisation on 26/02/2014.
//  Copyright (c) 2014. All rights reserved.
//

#import "AppDelegate.h"
#import <Mush Core/mushLog.hpp>
#import <AppKit/NSTabViewItem.h>

#include <RadeonLibrary/radeonConfig.hpp>
#include <ParFramework/parConfig.hpp>
#include <sstream>

#import "mushConfig.h"
/*
#import <Cocoa/Cocoa.h>
@interface NSColor(NSColorHexadecimalValue)
-(NSString *)hexadecimalValueOfAnNSColor;
@end

@implementation NSColor(NSColorHexadecimalValue)

-(NSString *)hexadecimalValueOfAnNSColor
{
    CGFloat redFloatValue, greenFloatValue, blueFloatValue;
    int redIntValue, greenIntValue, blueIntValue;
    NSString *redHexValue, *greenHexValue, *blueHexValue;
    
    //Convert the NSColor to the RGB color space before we can access its components
    NSColor *convertedColor=[self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
    
    if(convertedColor)
    {
        // Get the red, green, and blue components of the color
        [convertedColor getRed:&redFloatValue green:&greenFloatValue blue:&blueFloatValue alpha:NULL];
        
        // Convert the components to numbers (unsigned decimal integer) between 0 and 255
        redIntValue=redFloatValue*255.99999f;
        greenIntValue=greenFloatValue*255.99999f;
        blueIntValue=blueFloatValue*255.99999f;
        
        // Convert the numbers to hex strings
        redHexValue=[NSString stringWithFormat:@"%02x", redIntValue];
        greenHexValue=[NSString stringWithFormat:@"%02x", greenIntValue];
        blueHexValue=[NSString stringWithFormat:@"%02x", blueIntValue];
        
        // Concatenate the red, green, and blue components' hex strings together with a "#"
        return [NSString stringWithFormat:@"#%@%@%@", redHexValue, greenHexValue, blueHexValue];
    }
    return nil;
}
@end
*/
@interface AppDelegate () {
    NSMutableArray *_frameInputs;
    NSMutableArray *_videoInputs;
    NSMutableArray *_videoOutputs;
    
    NSMutableArray *_videoMultipleInputs;
    NSMutableArray *_videoMultipleOutputs;
    
    bool _has_opened_queue;
}
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	[[_userDefaults defaults] registerDefaults:(@{
												  @"gamma" : @"1.0",
												  @"exposure" : @"0.0",
												  })];
    
    _has_opened_queue = false;

}

- (void)awakeFromNib {
	[_showGUI	addObserver:self
				forKeyPath:@"cell.state"
				options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
				context:NULL];
	if ([_showGUI state]) {
		[_sim2Preview setEnabled: true];
	} else {
		[_sim2Preview setEnabled: false];
	}
    
    if ([_overrideSize state]) {
        [_overrideWidth setEnabled:YES];
        [_overrideHeight setEnabled:YES];
    } else {
        [_overrideWidth setEnabled:NO];
        [_overrideHeight setEnabled:NO];
    }
    
    [_folderPathButton initAttachDefaults:_userDefaults withKey:@"frameInputs" isSave:FALSE];
    [_secondFolderPath initAttachDefaults:_userDefaults withKey:@"second_folder_input" isSave:FALSE];
    [_thirdFolderPath initAttachDefaults:_userDefaults withKey:@"third_folder_input" isSave:FALSE];
    [_fourthFolderPath initAttachDefaults:_userDefaults withKey:@"fourth_folder_input" isSave:FALSE];
    [_exrPathButton initAttachDefaults:_userDefaults withKey:@"exr_inputs" isSave:TRUE];
    [_yuv12bitPathButton initAttachDefaults:_userDefaults withKey:@"yuv_inputs" isSave:TRUE];
    [_singleImagePath initAttachDefaults:_userDefaults withKey:@"single_image_inputs" isSave:FALSE];
    [_secondStillBox initAttachDefaults:_userDefaults withKey:@"second_still_inputs" isSave:FALSE];
    [_thirdStillBox initAttachDefaults:_userDefaults withKey:@"third_still_inputs" isSave:FALSE];
    [_fourthStillBox initAttachDefaults:_userDefaults withKey:@"fourth_still_inputs" isSave:FALSE];
    
    
    [_singleImagePath setCanChooseFiles:YES];
    [_singleImagePath setCanChooseDirectories:NO];
    [_singleImagePath setCanChooseMultiple:NO];
    [_singleImagePath setFileExtension:@"exr"];
    
    [_secondStillBox setCanChooseFiles:YES];
    [_secondStillBox setCanChooseDirectories:NO];
    [_secondStillBox setCanChooseMultiple:NO];
    [_secondStillBox setFileExtension:@"exr"];
    
    [_thirdStillBox setCanChooseFiles:YES];
    [_thirdStillBox setCanChooseDirectories:NO];
    [_thirdStillBox setCanChooseMultiple:NO];
    [_thirdStillBox setFileExtension:@"exr"];
    
    [_fourthStillBox setCanChooseFiles:YES];
    [_fourthStillBox setCanChooseDirectories:NO];
    [_fourthStillBox setCanChooseMultiple:NO];
    [_fourthStillBox setFileExtension:@"exr"];
    
    [_folderPathButton setCanChooseFiles:NO];
    [_folderPathButton setCanChooseDirectories:YES];
    [_folderPathButton setCanChooseMultiple:NO];
    
    [_secondFolderPath setCanChooseFiles:NO];
    [_secondFolderPath setCanChooseDirectories:YES];
    [_secondFolderPath setCanChooseMultiple:NO];
    
    [_thirdFolderPath setCanChooseFiles:NO];
    [_thirdFolderPath setCanChooseDirectories:YES];
    [_thirdFolderPath setCanChooseMultiple:NO];
    
    [_fourthFolderPath setCanChooseFiles:NO];
    [_fourthFolderPath setCanChooseDirectories:YES];
    [_fourthFolderPath setCanChooseMultiple:NO];
    
    [_secondVideoPath initAttachDefaults:_userDefaults withKey:@"second_video_path" isSave:FALSE];
    [_secondVideoPath setCanChooseFiles:YES];
    [_secondVideoPath setCanChooseDirectories:NO];
    [_secondVideoPath setCanChooseMultiple:NO];
    
    [_genericCameraPath initAttachDefaults:_userDefaults withKey:@"generic_camera_path" isSave:FALSE];
    [_genericCameraPath setCanChooseFiles:YES];
    [_genericCameraPath setCanChooseDirectories:NO];
    [_genericCameraPath setCanChooseMultiple:NO];
    [_genericCameraPath setFileExtension:@"json"];
    
    
    [_genericSaveCameraPath initAttachDefaults:_userDefaults withKey:@"generic_camera_save_path" isSave:TRUE];
    [_genericSaveCameraPath setCanChooseFiles:YES];
    [_genericSaveCameraPath setCanChooseDirectories:NO];
    [_genericSaveCameraPath setCanChooseMultiple:NO];
    [_genericSaveCameraPath setFileExtension:@"json"];
    
    
    [_modelPath initAttachDefaults:_userDefaults withKey:@"model_path" isSave:FALSE];
    [_modelPath setCanChooseFiles:YES];
    [_modelPath setCanChooseDirectories:NO];
    [_modelPath setCanChooseMultiple:NO];
    [_modelPath setFileExtension:@"obj"];
    
    //[_amdObj setFileExtension:@"obj"];
    [_exrPathButton setFileExtension:@"exr"];
    [_yuv12bitPathButton setFileExtension:@"yuv12"];
    
    _videoInputs = [[NSMutableArray alloc] init];
    _videoOutputs = [[NSMutableArray alloc] init];
    
    
    _videoMultipleInputs = [[NSMutableArray alloc] init];
    _videoMultipleOutputs = [[NSMutableArray alloc] init];
    
    if (![[_userDefaults defaults]	arrayForKey:@"frameInputs"]) {
        NSLog(@"no frame defaults");
        [[_userDefaults defaults] setObject:_videoInputs forKey:@"videoInputs"];
        [[_userDefaults defaults] setObject:_videoOutputs forKey:@"videoOutputs"];
    } else {
        [_videoInputs addObjectsFromArray:[[_userDefaults defaults] arrayForKey:@"videoInputs"]] ;
        [_videoOutputs addObjectsFromArray:[[_userDefaults defaults] arrayForKey:@"videoOutputs"]] ;
    }
    
    NSOrderedSet *tmp2 = [NSOrderedSet orderedSetWithArray:_videoInputs];
    _videoInputs = [[tmp2 array] mutableCopy];
    NSOrderedSet *tmp3 = [NSOrderedSet orderedSetWithArray:_videoOutputs];
    _videoOutputs = [[tmp3 array] mutableCopy];
    
    if ([_videoInputs count] > 10) {
        _videoInputs = [[_videoInputs subarrayWithRange:NSMakeRange([_videoInputs count] - 10, 10)] mutableCopy];
    }
    
    if ([_videoOutputs count] > 10) {
        _videoOutputs = [[_videoOutputs subarrayWithRange:NSMakeRange([_videoOutputs count] - 10, 10)] mutableCopy];
    }
    
    [[_userDefaults defaults] setObject:_videoInputs forKey:@"videoInputs"];
    [[_userDefaults defaults] setObject:_videoOutputs forKey:@"videoOutputs"];
    
    [_videoPathButton addItemsWithTitles:_videoInputs];
    [_ffmpegPathButton addItemsWithTitles:_videoOutputs];
    
    /*
    [_inputMethod removeTabViewItem:([_inputMethod tabViewItemAtIndex:4])];
    [_inputMethod removeTabViewItem:([_inputMethod tabViewItemAtIndex:2])];
    
	[_outputMethod removeTabViewItem:([_outputMethod tabViewItemAtIndex:3])];
	[_outputMethod removeTabViewItem:([_outputMethod tabViewItemAtIndex:2])];
    
    [_processMethod removeTabViewItem:([_processMethod tabViewItemAtIndex:3])];
    [_processMethod removeTabViewItem:([_processMethod tabViewItemAtIndex:2])];
     
    //[_processMethod removeTabViewItem:([_processMethod tabViewItemAtIndex:0])];
    
    [_demoMode setEnabled:false];
    
    [_genericChoice setEnabled:false];
    */
    
    if ([_dualISO state]){
        _firstISO.placeholderString = @"Scale";
        _secondISO.placeholderString = @"Frame Skip";
        _thirdISO.placeholderString = @"WB Red";
        _fourthISO.placeholderString = @"WB Blue";
    }
    
    [self livestream_checked:self];
    [self resize_checked:self];
    
    
    self.dataOutput = [[dataOutputWindow alloc] initWithWindowNibName:@"dataOutputWindow"];
    
    self.mushQueueWindow = [[mushQueue alloc] initWithWindowNibName:@"mushQueue"];
    
    self.openglWindow = [[openglWindowController alloc] initWithWindowNibName:@"openglWindowController"];
    
    //[self.dataOutput showWindow:self];
    //[self.dataOutput.window orderOut:self.dataOutput.window];
    
    //[[self.dataOutput window] orderOut:self];
    _textOutput.automaticQuoteSubstitutionEnabled = FALSE;
    
    
}


- (void)dealloc
{
    [_showGUI removeObserver:self forKeyPath:@"cell.state"];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([keyPath isEqual:@"cell.state"]) {
        if ([[change objectForKey: @"new"] integerValue] == 1) {
			[_sim2Preview setEnabled: true];
		} else {
			[_sim2Preview setEnabled: false];
		}
    }
}

- (IBAction)ffmpegPathButton:(id)sender {
	NSSavePanel* saveDlg = [NSSavePanel savePanel];
	[saveDlg setCanCreateDirectories:YES];
	NSString *ext = @"mp4";
    switch ([[[_processMethod selectedTabViewItem] identifier] integerValue]) {
		case 0:
			ext = @"hdrv";
			break;
		case 2:
			ext = @"exr";
            break;
        default:
            
            if ([[_ffmpegEncoder title] isEqual: @"VP9"]) {
                ext = @"webm";
            } else if ([[_ffmpegEncoder title] isEqual: @"Prores"]) {
                ext = @"mov";
            } else {
                ext = @"mp4";
            }
            break;
	}
	[saveDlg setAllowedFileTypes:@[ ext ] ];
	if ( [saveDlg runModal] == NSFileHandlingPanelOKButton )
	{
		NSURL *url = [saveDlg URL];
		if ([url isFileURL]) {
			NSString* fileName = [url path];
			[_ffmpegPathButton addItemWithTitle: fileName];
            [_ffmpegPathButton selectItemWithTitle: fileName];
            
            [_videoOutputs addObject:fileName];
            [[_userDefaults defaults] setObject:_videoOutputs forKey:@"videoOutputs"];
		}
	} else {
		[_ffmpegPathButton selectItemAtIndex:0];
	}
}

- (IBAction)videoPathButton:(id)sender {
	NSOpenPanel* openDlg = [NSOpenPanel openPanel];
	[openDlg setCanChooseFiles:YES];
	[openDlg setCanChooseDirectories:NO];
	[openDlg setAllowsMultipleSelection:YES];
    
    [_videoMultipleInputs removeAllObjects];
    [_videoMultipleOutputs removeAllObjects];
    
	if ( [openDlg runModal] == NSFileHandlingPanelOKButton )
	{
        if ([openDlg URLs].count > 1) {
            for (int i = 0; i < [openDlg URLs].count; ++i) {
                
                NSURL *url = [openDlg URLs][i];
                if ([url isFileURL]) {
                    NSString* fileName = [url path];
                    /*
                    [_videoPathButton addItemWithTitle: fileName];
                    [_videoPathButton selectItemWithTitle: fileName];
                    */
                    /*
                    [_videoInputs addObject:fileName];
                    [[_userDefaults defaults] setObject:_videoInputs forKey:@"videoInputs"];
                    */
                    
                    [_videoMultipleInputs addObject:fileName];
                    
                    
                    NSString * ffmpeg_path = [[[_ffmpegPathButton title] stringByDeletingLastPathComponent] stringByAppendingPathComponent:[[[[url path] lastPathComponent] stringByDeletingPathExtension] stringByAppendingPathExtension:[[_ffmpegPathButton title] pathExtension]]];
                    [_videoMultipleOutputs addObject:ffmpeg_path];
                    
                    
                }
                
            }
            
            [_videoPathButton addItemWithTitle: @"Multiple Selections"];
            [_videoPathButton selectItemWithTitle: @"Multiple Selections"];
            
            [_ffmpegPathButton addItemWithTitle: @"Multiple Selections"];
            [_ffmpegPathButton selectItemWithTitle: @"Multiple Selections"];
            
        } else {
            NSURL *url = [openDlg URL];
            if ([url isFileURL]) {
                NSString* fileName = [url path];
                [_videoPathButton addItemWithTitle: fileName];
                [_videoPathButton selectItemWithTitle: fileName];
                
                [_videoInputs addObject:fileName];
                [[_userDefaults defaults] setObject:_videoInputs forKey:@"videoInputs"];
                
                
                NSString * ffmpeg_path = [[[_ffmpegPathButton title] stringByDeletingLastPathComponent] stringByAppendingPathComponent:[[[[url path] lastPathComponent] stringByDeletingPathExtension] stringByAppendingPathExtension:[[_ffmpegPathButton title] pathExtension]]];
                
                
                [_ffmpegPathButton addItemWithTitle: ffmpeg_path];
                [_ffmpegPathButton selectItemWithTitle: ffmpeg_path];
                
                [_videoOutputs addObject:ffmpeg_path];
                [[_userDefaults defaults] setObject:_videoOutputs forKey:@"videoOutputs"];
                
                
            }
        }
	} else {
		[_videoPathButton selectItemAtIndex:0];
	}
}

- (IBAction)encodeClicked:(id)sender {
	
	mush::config config = [self getConfig];
    
    
    
    [self runConfig:config];
}

- (bool)runConfig:(mush::config)config {
    
    static dispatch_once_t once;
    dispatch_once(&once, ^ {
        
        dispatch_block_t myBlock = ^{
            self.timer = [NSTimer timerWithTimeInterval:0.1 target:self selector:@selector(logStack) userInfo:nil repeats:YES];
            //		[self.timer fire];
            [[NSRunLoop mainRunLoop] addTimer:self.timer forMode:NSDefaultRunLoopMode];
            
            self.timer2 = [NSTimer timerWithTimeInterval:0.1 target:self selector:@selector(rowStack) userInfo:nil repeats:YES];
            //		[self.timer fire];
            [[NSRunLoop mainRunLoop] addTimer:self.timer2 forMode:NSDefaultRunLoopMode];
        };
        
        
        dispatch_queue_t queue = dispatch_get_main_queue();
        
        dispatch_async(queue, myBlock);
        
    });
    /*	NSWindow * window = ((NSControl *)sender).window;
     NSRect rect = [window frame];
     NSRect rectNewFrame = [window frameRectForContentRect:NSMakeRect(0, 0, rect.size.width+200, rect.size.height)];
     rectNewFrame.origin = NSMakePoint(
     rect.origin.x + .5f * (rect.size.width - rectNewFrame.size.width),
     rect.origin.y + .5f * (rect.size.height - rectNewFrame.size.height));
     [window setFrame:rectNewFrame display:YES animate:YES];
     */
    
    
    //    if ([[[_inputMethod selectedTabViewItem] identifier] integerValue] == 3) {
    //        videoMushPresetAMDCameras(&config);
    //    } else {
    bool success = TRUE;
    
    config.processEngine = _processEngine;
    if ([_catchExceptions state] == true) {
        try {
            [self execute:config];
            /*} catch (cl::Error &e) {
             std::stringstream strm;
             strm << "Exception: " << e.what() << " " << e.err();
             putLog(strm.str());*/
        } catch (std::exception &e) {
            putLog(e.what());
            videoMushDestroy();
            success = false;
        }
    } else {
        [self execute:config];
    }
    
    return success;
    
    //[self.timer invalidate];
}

- (void)execute:(mush::config)config {
    if ([_videoMultipleInputs count] > 1) {
        for (int i = 0; i < [_videoMultipleInputs count]; ++i) {
            
            std::stringstream strm;
            strm << "Processing video " << i+1 << " of " << [_videoMultipleInputs count] << ".";
            putLog(strm.str());
            
            NSString * inp = _videoMultipleInputs[i];
            NSString * oup_p = _videoMultipleOutputs[i];
            
            config.outputConfig.outputPath = [[oup_p stringByDeletingLastPathComponent] UTF8String];
            config.outputConfig.outputName = [[oup_p lastPathComponent] UTF8String];
            
            config.inputConfig.inputPath = [inp UTF8String];
            
            videoMushRunAll(&config);
            int size = videoMushGetOutputCount();
            std::vector<uint8_t> use_outputs;
            for (int i = 0; i < size; ++i) {
                char str[1024];
                videoMushGetOutputName(str, 1024);
                putLog(str);
                use_outputs.push_back(true);
            }
            bool * o = (bool *)&use_outputs[0];
            videoMushExecute(size, &o);
        }
    } else {
        videoMushRunAll(&config);
        int size = videoMushGetOutputCount();
        std::vector<uint8_t> use_outputs;
        for (int i = 0; i < size; ++i) {
            char str[1024];
            videoMushGetOutputName(str, 1024);
            putLog(str);
            use_outputs.push_back(true);
        }
        bool * o = (bool *)&use_outputs[0];
        videoMushExecute(size, &o);
    }
}

- (void)logStack {
	char logs[4096];
    bool done = getLog(logs, 4096);
    bool first_done = done;
    
	while (done) {
		NSAttributedString* attr = [[NSAttributedString alloc] initWithString:
									[NSString stringWithFormat: @"%s\n", logs]];

        
		[[_statsBox textStorage] appendAttributedString:attr];
        [_statsBox scrollRangeToVisible:NSMakeRange([[_statsBox string] length], 0)];
        done = getLog(logs, 4096);
	}
    if (first_done) {
        [_statsBox setEditable:YES];
        [_statsBox setFont:[NSFont userFixedPitchFontOfSize:9.0]];
        [_statsBox setEditable:NO];
    }
    
    
}

- (void)rowStack {
    NSMutableArray * names_strings = [[NSMutableArray alloc] init];
    uint32_t names_size = 0;
    if (newRowNames(&names_size)) {
        for (UInt32 i = 0; i < names_size; ++i) {
            char name_string[128];
            getRowName(name_string, i, 128);
            [names_strings addObject:[NSString stringWithUTF8String:name_string]];
        }
        
        [_dataOutput setNames:names_strings];
        
    }
    
    uint32_t row_size = getRowCount();
    if (row_size > 0)
    {
        mush::metric_value * arr = new mush::metric_value[row_size];
        getRow(arr, row_size);
        [_dataOutput addRow:arr array_size:row_size];
        delete[] arr;
    }
    
}

/*
 
 while (th.IsAlive)
 {
 UInt32 names_size = 0;
 if (DataOutput.newRowNames(ref names_size))
 {
 StringBuilder[] name_strings = new StringBuilder[names_size];
 for (UInt32 i = 0; i < names_size; ++i) {
 name_strings[i] = new StringBuilder(128);
 DataOutput.getRowName(name_strings[i], i, 128);
 }
 
 data_output_window.AddRowNames(name_strings);
 }
 UInt32 row_size = DataOutput.getRowCount();
 if (row_size > 0)
 {
 mush.MetricValue[] arr = new mush.MetricValue[row_size];
 DataOutput.getRow(arr, row_size);
 data_output_window.AddRow(arr);
 }
 }
 UInt32 last_row_size = DataOutput.getRowCount();
 while (last_row_size > 0)
 {
 mush.MetricValue[] arr = new mush.MetricValue[last_row_size];
 DataOutput.getRow(arr, last_row_size);
 data_output_window.AddRow(arr);
 
 last_row_size = DataOutput.getRowCount();
 }
 DataOutput.endRow();
 */


- (IBAction)statsClicked:(id)sender {
 
}

- (mush::config)getConfig {
	
	mush::config config;
	config.defaults();
    
	long i = [[[_inputMethod selectedTabViewItem] identifier] integerValue];
	switch (i) {
		case 1:
			config.inputConfig.inputEngine = mush::inputEngine::folderInput;
            config.inputConfig.loopFrames = [_framesLoop state];
 
            if ([[_secondISO stringValue] length] > 0) {
                config.inputConfig.frame_skip = [_secondISO integerValue];
            }
            
			break;
		case 2:
            config.inputConfig.inputEngine = mush::inputEngine::videoInput;
            
            config.inputConfig.loopFrames = [_videoLoop state];
            
            switch ([_whiteBalance indexOfSelectedItem]) {
                case 0: // tungsten
                    config.inputConfig.whitePoint[0] = 1.392498;
                    config.inputConfig.whitePoint[1] = 1.000000;
                    config.inputConfig.whitePoint[2] = 2.375114;
                    break;
                case 1: // daylight
                    config.inputConfig.whitePoint[0] = 2.132483;
                    config.inputConfig.whitePoint[1] = 1.000000;
                    config.inputConfig.whitePoint[2] = 1.480864;
                    break;
                case 2: // fluorescent
                    config.inputConfig.whitePoint[0] = 1.783446;
                    config.inputConfig.whitePoint[1] = 1.000000;
                    config.inputConfig.whitePoint[2] = 1.997113;
                    break;
                case 3: // Shade
                    config.inputConfig.whitePoint[0] = 2.531894;
                    config.inputConfig.whitePoint[1] = 1.000000;
                    config.inputConfig.whitePoint[2] = 1.223749;
                    break;
                case 4: // flash
                    config.inputConfig.whitePoint[0] = 2.429833;
                    config.inputConfig.whitePoint[1] = 1.000000;
                    config.inputConfig.whitePoint[2] = 1.284593;
                    break;
                case 5:// cloudy
                    config.inputConfig.whitePoint[0] = 2.336605;
                    config.inputConfig.whitePoint[1] = 1.000000;
                    config.inputConfig.whitePoint[2] = 1.334642;
                    break;
                case 6: // custom
                    config.inputConfig.whitePoint[0] = 1.392498;
                    config.inputConfig.whitePoint[1] = 1.000000;
                    config.inputConfig.whitePoint[2] = 2.375114;
                    if ([[_thirdISO stringValue] length] > 0) {
                        config.inputConfig.whitePoint[0] = [_thirdISO floatValue];
                    }
                    if ([[_fourthISO stringValue] length] > 0) {
                        config.inputConfig.whitePoint[2] = [_fourthISO floatValue];
                    }
                    break;
            }
            /*
             
             Tungsten: multipliers 1.392498 1.000000 2.375114
             Daylight: multipliers 2.132483 1.000000 1.480864
             Fluorescent: multipliers 1.783446 1.000000 1.997113
             Shade: multipliers 2.531894 1.000000 1.223749
             Flash: multipliers 2.429833 1.000000 1.284593
             Cloudy: multipliers 2.336605 1.000000 1.334642"
             
             */
            if ([[_firstISO stringValue] length] > 0) {
                config.inputConfig.dual_iso_comp_factor = [_firstISO floatValue];
            }
            if ([[_secondISO stringValue] length] > 0) {
                config.inputConfig.frame_skip = [_secondISO integerValue];
            }
			break;
		case 3:
            config.inputConfig.inputEngine = mush::inputEngine::canonInput;
			config.inputConfig.inputWidth = 1920;
            config.inputConfig.inputHeight = 1080;
            break;
        case 4:
            config.inputConfig.inputEngine = mush::inputEngine::flare10Input;
            switch ([_flareResolution selectedColumn]) {
                case 0:
                    config.inputConfig.bmMode = mush::bmModes::m720p60;
                    break;
                case 1:
                    config.inputConfig.bmMode = mush::bmModes::m1080p24;
                    break;
                case 2:
                    config.inputConfig.bmMode = mush::bmModes::m1080p25;
                    break;
                case 3:
                    config.inputConfig.bmMode = mush::bmModes::m1080p30;
                    break;
            }
            
            if (![_flareSerial state]) {
                config.inputConfig.flareCOMPort = "";
            } else {
                config.inputConfig.flareCOMPort = [[_flareSerialPath stringValue] UTF8String];
            }
            
			break;
        case 5:
            config.inputConfig.inputEngine = mush::inputEngine::noInput;
            break;
        case 6:
            config.inputConfig.inputEngine = mush::inputEngine::singleEXRInput;
            break;
	}
	
	switch([_exposureCount selectedColumn]) {
		case 0:
			config.inputConfig.exposures = 1;
			break;
		case 1:
			config.inputConfig.exposures = 2;
			break;
		case 2:
			config.inputConfig.exposures = 3;
			break;
		case 3:
			config.inputConfig.exposures = 4;
			break;
	}
	
	switch([[[_outputMethod selectedTabViewItem] identifier] integerValue]) {
		case 4:
        {
            switch ([_imageFileType indexOfSelectedItem]) {
                case 0:
                    config.outputConfig.encodeEngine = mush::encodeEngine::exr;
                    break;
                case 1:
                    config.outputConfig.encodeEngine = mush::encodeEngine::gif;
                    break;
            }
        }
            
            if ([_ffmpegFPS floatValue]) {
                config.outputConfig.fps = [_ffmpegFPS floatValue];
            }
            
            config.outputConfig.outputEngine = mush::outputEngine::noOutput;
            config.outputConfig.outputPath = [[[_exrPathButton title] stringByDeletingLastPathComponent] UTF8String];
            config.outputConfig.outputName = [[[_exrPathButton title] lastPathComponent] UTF8String];
			break;
		case 5:
//			config.outputConfig.outputEngine = mush::outputEngine::screenOutput;
			break;
		case 6:
			config.outputConfig.outputEngine = mush::outputEngine::libavformatOutput;
            if ([_dummyRun state] == TRUE) {
                config.outputConfig.outputEngine = mush::outputEngine::noOutput;
            }
			config.outputConfig.outputPath = [[[_ffmpegPathButton title] stringByDeletingLastPathComponent] UTF8String];
			config.outputConfig.outputName = [[[_ffmpegPathButton title] lastPathComponent] UTF8String];
            
            if ([_ffmpegFPS floatValue]) {
                config.outputConfig.fps = [_ffmpegFPS floatValue];
            }
            if ([_crfBox intValue]) {
                config.outputConfig.crf = [_crfBox intValue];
            }
            
            if ([[_ffmpegEncoder title] isEqual: @"x264"]) {
                config.outputConfig.encodeEngine = mush::encodeEngine::x264;
            }
            
            if ([[_ffmpegEncoder title] isEqual: @"x265"]) {
                config.outputConfig.encodeEngine = mush::encodeEngine::x265;
            }
            
            if ([[_ffmpegEncoder title] isEqual: @"VP9"]) {
                config.outputConfig.encodeEngine = mush::encodeEngine::vpx;
            }
            
            if ([[_ffmpegEncoder title] isEqual: @"Prores"]) {
                config.outputConfig.encodeEngine = mush::encodeEngine::prores;
            }
        
            switch ([_ffmpegTransfer indexOfSelectedItem]) {
                case 0:
                default:
                config.outputConfig.func = mush::transfer::srgb;
                break;
                case 1:
                config.outputConfig.func = mush::transfer::g8;
                break;
                case 2:
                config.outputConfig.func = mush::transfer::pq;
                break;
                case 3:
                config.outputConfig.func = mush::transfer::linear;
                    break;
                case 4:
                    config.outputConfig.func = mush::transfer::rec709;
                    break;
                
            }
            
            config.outputConfig.liveStream.enabled = [_useHLS state];
            if (config.outputConfig.liveStream.enabled) {
                config.outputConfig.zerolatency = true;
                
                if (self.liveStreamWindow != nil) {
                    config.outputConfig.liveStream.webroot = [[[[_liveStreamWindow webroot] selectedItem ] title] UTF8String];
                    config.outputConfig.liveStream.webaddress = [[[_liveStreamWindow webaddress] stringValue] UTF8String];
                    config.outputConfig.liveStream.streamname = [[[_liveStreamWindow streamname] stringValue] UTF8String];
                    config.outputConfig.liveStream.num_files = [[_liveStreamWindow num_files] integerValue];
                    config.outputConfig.liveStream.file_length = [[_liveStreamWindow file_length] integerValue];
                    config.outputConfig.liveStream.wrap_after = [[_liveStreamWindow wrap_after] integerValue];
                }
            }
			break;
        case 7:
            config.outputConfig.encodeEngine = mush::encodeEngine::none;
			config.outputConfig.outputEngine = mush::outputEngine::noOutput;
            break;
            
        case 8:
            config.outputConfig.encodeEngine = mush::encodeEngine::yuvRaw;
            config.outputConfig.outputEngine = mush::outputEngine::noOutput;
            config.outputConfig.outputPath = [[[_yuv12bitPathButton title] stringByDeletingLastPathComponent] UTF8String];
            config.outputConfig.outputName = [[[_yuv12bitPathButton title] lastPathComponent] UTF8String];
            break;
            
	}
    
    if ([_resizeCheck state]) {
        config.inputConfig.resize = true;
        config.inputConfig.resize_width = [_input_resize_width integerValue];
        config.inputConfig.resize_height = [_input_resize_height integerValue];
    }
    
    config.inputConfig.darken = [_exposureValue floatValue];
    
    config.demoMode = [_demoMode state];
    
    config.outputConfig.use_auto_decode = [_autoDecodeCheck state];
    
    config.outputConfig.exr_timestamp = [_exrTimestamp state];
    
    switch ([_inputTransfer indexOfSelectedItem]) {
        case 0:
        default:
        config.inputConfig.func = mush::transfer::srgb;
        break;
        case 1:
        config.inputConfig.func = mush::transfer::g8;
        break;
        case 2:
        config.inputConfig.func = mush::transfer::pq;
        break;
        case 3:
        config.inputConfig.func = mush::transfer::linear;
        break;
        case 4:
        config.inputConfig.func = mush::transfer::logc;
            break;
        case 5:
            config.inputConfig.func = mush::transfer::rec709;
            break;
        
    }
    
    
    if([_testCard state]) {
        config.inputConfig.inputEngine = mush::inputEngine::testCardInput;
    }
    
	switch ([[[_processMethod selectedTabViewItem] identifier] integerValue]) {
		case 0:
			_processEngine = mush::processEngine::none;
			break;
		case 1:
			_processEngine = mush::processEngine::tonemapCompress;
			break;
		case 3:
			_processEngine = mush::processEngine::none;
			break;
		case 4:
			_processEngine = mush::processEngine::none;
            break;
        case 5:
            _processEngine = mush::processEngine::slic;
            
            config.slicConfig.slicDrawBorders = [_slicCellBorders state];
            config.slicConfig.slicDrawCenters = [_slicCellCenters state];
            config.slicConfig.slicFillCells = [_slicFillCells state];
            config.slicConfig.slicDrawUniqueness = [_slicUniqueness state];
            config.slicConfig.slicDrawDistribution = [_slicDistribution state];
            config.slicConfig.slicDrawSaliency = [_slicSaliency state];
            break;
        case 6:
            _processEngine = mush::processEngine::waveform;
            break;
        case 7:
            _processEngine = mush::processEngine::sand;
            config.inputConfig.inputWidth = 1280;
            config.inputConfig.inputHeight = 720;
            break;
        case 8:
            _processEngine = mush::processEngine::fixedBitDepth;
			switch ([_transferFunc selectedTag]) {
				case 0:
				default:
					config.func = mush::transfer::linear;
					break;
				case 1:
					config.func = mush::transfer::g8;
					break;
				case 2:
					config.func = mush::transfer::pq;
					break;
			}
            
            switch ([_fbdOutput indexOfSelectedItem]) {
                case 0:
                    config.fbd_output = mush::fbdOutputs::decoded;
                    break;
                case 1:
                    config.fbd_output = mush::fbdOutputs::chromaSwap;
                    break;
                case 2:
                    config.fbd_output = mush::fbdOutputs::banding;
                    break;
                case 3:
                    config.fbd_output = mush::fbdOutputs::falseColour;
                    break;
                case 4:
                    config.fbd_output = mush::fbdOutputs::switcher;
                    break;
                    
            }
			
            config.outputConfig.yuvBitDepth = [_yuvBitDepth floatValue];
            config.outputConfig.yuvMax = [_yuvMaxValue floatValue];
            config.outputConfig.pqLegacy = [_pqLegacy state];
            break;
        case 9:
            _processEngine = mush::processEngine::generic;
            
            switch ([_genericChoice indexOfSelectedItem]) {
                case 0:
                    _processEngine = mush::processEngine::none;
                    break;
                case 1:
                    config.inputConfig.inputEngine = mush::inputEngine::noInput;
                    _processEngine = mush::processEngine::sand;
                    config.generatorConfig.type = mush::generatorProcess::sand;
                    config.inputConfig.inputWidth = 1280;
                    config.inputConfig.inputHeight = 720;
                    break;
                case 2: // SEPARATOR
                case 3:
                    _processEngine = mush::processEngine::none;
                    break;
                case 4: // SEPARATOR
                case 5:
                    _processEngine = mush::processEngine::waveform;
                    break;
                case 6:
                    config.genericChoice = mush::genericProcess::barcode;
                    break;
                case 7:
                    config.genericChoice = mush::genericProcess::displaceChannel;
                    break;
                case 8:
                    config.genericChoice = mush::genericProcess::rotateImage;
                    break;
                case 9:
                    config.genericChoice = mush::genericProcess::falseColour;
                    break;
                case 10:
                    config.genericChoice = mush::genericProcess::bt709luminance;
                    break;
                case 11:
                    config.genericChoice = mush::genericProcess::tonemap;
                    break;
                case 12:
                    config.genericChoice = mush::genericProcess::colourBilateral;
                    break;
                case 13:
                    config.genericChoice = mush::genericProcess::laplace;
                    break;
                case 14:
                    _processEngine = mush::processEngine::debayer;
                    break;
                case 15:
                    config.genericChoice = mush::genericProcess::tape;
                    break;
                case 16:
                    _processEngine = mush::processEngine::trayrace;
                    break;
                case 17:
                    _processEngine = mush::processEngine::sand;
                    config.generatorConfig.type = mush::generatorProcess::sphere;
                    config.inputConfig.inputWidth = 1920;
                    config.inputConfig.inputHeight = 1080;
                    
                    break;
                case 18:
                    config.genericChoice = mush::genericProcess::fisheye2equirectangular;
                    break;
                case 19:
                    config.genericChoice = mush::genericProcess::sobel;
                    break;
                case 20:
                    _processEngine = mush::processEngine::sand;
                    config.generatorConfig.type = mush::generatorProcess::motionReprojection;
                    /*
                    config.inputConfig.secondInputPath = [[_secondStillBox title] UTF8String];
                    config.inputConfig.thirdInputPath = [[_thirdStillBox title] UTF8String];
                    config.inputConfig.fourthInputPath = [[_fourthStillBox title] UTF8String];
                     */
                    
                    break;
                case 21:
                    _processEngine = mush::processEngine::sand;
                    config.generatorConfig.type = mush::generatorProcess::anaglyph;
                    config.metric_path = [[_secondStillBox title] UTF8String];
                    break;
                case 22:
                    _processEngine = mush::processEngine::metrics;
                    //config.metric_path = [[_secondStillBox title] UTF8String];
                    break;
                case 23:
                    _processEngine = mush::processEngine::motionExplorer;
                    
                    config.parConfig = [self get_par_config];
                    /*
                    config.inputConfig.secondInputPath = [[_secondStillBox title] UTF8String];
                    config.inputConfig.thirdInputPath = [[_thirdStillBox title] UTF8String];
                    config.inputConfig.fourthInputPath = [[_fourthStillBox title] UTF8String];
                     */
                    break;
                case 24:
                    _processEngine = mush::processEngine::sand;
                    config.generatorConfig.type = mush::generatorProcess::motionGenerator;
                    
                    config.cameraConfig.autocam = [_genericAutoCamera state];
                    config.cameraConfig.load_path = [[[_genericCameraPath selectedItem] title] UTF8String];
                    config.cameraConfig.speed = [_genericSpeedFactor floatValue];
                    
                    switch ([_genericCameraType indexOfSelectedItem]) {
                        case 0:
                            config.parConfig.camera_type = par_camera_type::perspective;
                            break;
                        case 1:
                            config.parConfig.camera_type = par_camera_type::spherical;
                            break;
                    }
                    
                    break;
                case 25:
                _processEngine = mush::processEngine::sand;
                config.generatorConfig.type = mush::generatorProcess::sbsPack;
                config.inputConfig.secondInputPath = [[_secondStillBox title] UTF8String];
                break;
                case 26:
                _processEngine = mush::processEngine::sand;
                config.generatorConfig.type = mush::generatorProcess::sbsUnpack;
                
                break;
                case 27:
                _processEngine = mush::processEngine::sand;
                config.generatorConfig.type = mush::generatorProcess::raster;
                
                break;
                
            }
            
            break;
        case 10:
        {
            _processEngine = mush::processEngine::sand;
            config.generatorConfig.type = mush::generatorProcess::text;
            config.generatorConfig.text_output_string = [[[_textOutput textStorage] string] UTF8String];
            /*
            NSData *decode = [[[_textOutput textStorage] string] dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
            
            NSString *ansi = [[NSString alloc] initWithData:decode encoding:NSASCIIStringEncoding];
            
            config.generatorConfig.text_output_string = [ansi UTF8String];
            */
            config.inputConfig.inputWidth = [_input_resize_width integerValue];
            config.inputConfig.inputHeight = [_input_resize_height integerValue];
            /*
            NSString * bg_colour_string = [[_bg_colour color] hexadecimalValueOfAnNSColor];
            NSString * text_colour_string = [[_text_colour color] hexadecimalValueOfAnNSColor];
            
            config.generatorConfig.bg_colour = [bg_colour_string UTF8String];
            config.generatorConfig.text_colour = [text_colour_string UTF8String];
             */
            NSColor *converted_bg_color=[[_bg_colour color] colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
            
            if(converted_bg_color) {
                // Get the red, green, and blue components of the color
                CGFloat r,g,b,a;
                [converted_bg_color getRed:&r green:&g blue:&b alpha:&a];
                
                config.generatorConfig.bg_colour[0] = r;
                config.generatorConfig.bg_colour[1] = g;
                config.generatorConfig.bg_colour[2] = b;
                config.generatorConfig.bg_colour[3] = a;
            }
            
            NSColor *converted_text_color=[[_text_colour color] colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
            
            if(converted_text_color) {
                // Get the red, green, and blue components of the color
                CGFloat r,g,b,a;
                [converted_text_color getRed:&r green:&g blue:&b alpha:&a];
                
                config.generatorConfig.text_colour[0] = r;
                config.generatorConfig.text_colour[1] = g;
                config.generatorConfig.text_colour[2] = b;
                config.generatorConfig.text_colour[3] = a;
            }
        }
            break;
        case 11:
        {
            _processEngine = mush::processEngine::par;
            
            config.parConfig = [self get_par_config];
            
            if ([_oculusCheck state] == TRUE) {
                _processEngine = mush::processEngine::oculusDraw;
                config.radeonConfig = [self get_radeon_config];
            }
            
        }
            break;
        case 12:
            _processEngine = mush::processEngine::amd;
            
            config.radeonConfig = [self get_radeon_config];
            
            break;
	}
    /*
    switch ([_waveformMode selectedColumn]) {
        case 0:
            config.waveformConfig.waveformMode = mush::waveformMode::luma;
            break;
        case 1:
            config.waveformConfig.waveformMode = mush::waveformMode::rgb;
            break;
        case 2:
            config.waveformConfig.waveformMode = mush::waveformMode::r;
            break;
        case 3:
            config.waveformConfig.waveformMode = mush::waveformMode::g;
            break;
        case 4:
            config.waveformConfig.waveformMode = mush::waveformMode::b;
            break;
    }
	*/
	config.isoArray[0] = 1.0f;
	config.isoArray[1] = 4.0f;
	config.isoArray[2] = 8.0f;
	config.isoArray[3] = 32.0f;
	
	if ([_firstISO floatValue]) {
		config.isoArray[0] = [_firstISO floatValue];
	}
	
	if ([_secondISO floatValue]) {
		config.isoArray[1] = [_secondISO floatValue];
	}
	
	if ([_thirdISO floatValue]) {
		config.isoArray[2] = [_thirdISO floatValue];
	}
	
	if ([_fourthISO floatValue]) {
		config.isoArray[3] = [_fourthISO floatValue];
	}
    
    
    if ([_overrideSize state]) {
        config.outputConfig.overrideSize = [_overrideSize state];
        config.outputConfig.width = [_overrideWidth integerValue];
        config.outputConfig.height = [_overrideHeight integerValue];
    }
	
    
    config.inputConfig.dualISO = [_dualISO state];
    
    config.inputConfig.lock_fps = [_lockInputFPS state];
    config.inputConfig.fps = [_lockFPSText floatValue];
    
	config.gui.sim2preview = [_sim2Preview state];
    config.gui.show_gui = [_showGUI state];
    config.gui.fullscreen = [_full_screen state];
//[	config.gammacorrect = [_gammaValue floatValue];
    
    config.processEngine = _processEngine;
    
    
    NSString * frDir = [NSString stringWithFormat:@"%@/%@", [[NSBundle mainBundle] bundlePath], @"Contents/Frameworks/Video Mush.framework"];
    NSString * resDir = [[[NSBundle bundleWithPath:frDir] resourcePath] stringByAppendingString:@"/"];
    config.resourceDir =[resDir UTF8String];
    config.inputConfig.resourceDir =[resDir UTF8String];
    
    config.gui.exrDir = [[NSHomeDirectory() stringByAppendingString:@"/Pictures/EXR/"] UTF8String];
    
    switch ([[[_inputMethod selectedTabViewItem] identifier] integerValue]) {
        case 1:
        config.inputConfig.inputPath = [[_folderPathButton titleOfSelectedItem] UTF8String];
        config.inputConfig.secondInputPath = [[_secondFolderPath titleOfSelectedItem] UTF8String];
        config.inputConfig.thirdInputPath = [[_thirdFolderPath titleOfSelectedItem] UTF8String];
        config.inputConfig.fourthInputPath = [[_fourthFolderPath titleOfSelectedItem] UTF8String];
        break;
        case 2:
            config.inputConfig.inputPath = [[_videoPathButton titleOfSelectedItem] UTF8String];
            config.inputConfig.secondInputPath = [[_secondVideoPath title] UTF8String];
        break;
        case 6:
        config.inputConfig.inputPath = [[_singleImagePath titleOfSelectedItem] UTF8String];
        
        config.inputConfig.secondInputPath = [[_secondStillBox title] UTF8String];
        config.inputConfig.thirdInputPath = [[_thirdStillBox title] UTF8String];
        config.inputConfig.fourthInputPath = [[_fourthStillBox title] UTF8String];
        break;
    }
    
    
    config.cameraConfig.model_path = [[_modelPath title] UTF8String];
    config.cameraConfig.load_path =[[[_genericCameraPath selectedItem] title] UTF8String];
    config.cameraConfig.save_path =[[[_genericSaveCameraPath selectedItem] title] UTF8String];
    config.cameraConfig.speed = [_genericSpeedFactor floatValue];
    config.cameraConfig.autocam = [_genericAutoCamera state];
    config.cameraConfig.model_scale = [_genericModelScale floatValue];

    config.cameraConfig.position_x = [_generic_camera_x floatValue];
    config.cameraConfig.position_y = [_generic_camera_y floatValue];
    config.cameraConfig.position_z = [_generic_camera_z floatValue];
    config.cameraConfig.position_theta = [_generic_camera_theta floatValue];
    config.cameraConfig.position_phi = [_generic_camera_phi floatValue];
    config.cameraConfig.fov = [_generic_camera_fov floatValue];
    
    config.cameraConfig.width = [_par_width integerValue];
    config.cameraConfig.height = [_par_height integerValue];
    
    config.cameraConfig.stereo = [_genericStereo state];
    
    config.cameraConfig.equirectangular = [_generic_spherical state];
    config.cameraConfig.quit_at_camera_path_end = [_genericQuitAtEnd state];
    
    config.motionExplorer.follow_stereo = [_reprojStereoPath state];
    config.motionExplorer.spherical = [_reprojStereoToPers state];
    
    
    config.parConfig.camera_config = config.cameraConfig;
    
	return config;
}

- (parConfigStruct)get_par_config
{
    
    
    parConfigStruct pConfig;
    
    pConfig.defaults();
    
    if (self.parSettingWindow != nil) {
        pConfig = [_parSettingWindow get_config];
    }
    
    if ([_sponza state] == TRUE) {
        pConfig.scene = SCENE_SPONZA;
    } else if ([_sibenik state] == TRUE) {
        pConfig.scene = SCENE_SIBENIK;
    } else if ([_office state] == TRUE) {
        pConfig.scene = SCENE_CONFERENCE;
    } else if ([_kitchen state] == TRUE) {
        pConfig.scene = SCENE_GENERIC;
        pConfig.scene_variable = SCENE_GENERIC_VARIABLE_KITCHEN;
    }
    
    pConfig.iterations = [_par_iterations integerValue];
    pConfig.offset = [_par_offset integerValue];
    pConfig.frame_count = [_par_count integerValue];
    
    pConfig.use_remote_render_method = false;
    
    pConfig.opengl_only = ([_par_opengl state] == TRUE);
    
    
    pConfig.opengl_only = ([_par_opengl state] == TRUE);
    
    pConfig.automatic_camera_opengl_playback = ([_par_opengl state] == TRUE);
    
    
    switch ([_parCameraType indexOfSelectedItem]) {
        case 0:
            pConfig.camera_type = par_camera_type::perspective;
            break;
        case 1:
            pConfig.camera_type = par_camera_type::spherical;
            break;
    }
    
    pConfig.use_360_player = [_par360player state];
    
    pConfig.use_mush_model = [_parUseMushModel state];
    
    pConfig.four_output = [_parFourOutput state];
    
    pConfig.flip_normals = [_parFlipNormals state];
    return pConfig;
    
}

- (mush::radeonConfig)get_radeon_config
{
    mush::radeonConfig radConfig;
    radConfig.defaults();
    
    radConfig.width = [_par_width integerValue];
    radConfig.height = [_par_height integerValue];
    
    radConfig.shadow_rays = [_amdShadow integerValue];
    radConfig.ao_rays = [_amdAORays integerValue];
    radConfig.num_bounces = [_amdBounces integerValue];
    
    radConfig.share_opencl = [_amdShareCL state];
    radConfig.ao_enabled = [_amdAOEnable state];
    radConfig.environment_map_fish_eye = [_amdProgressive state];
    
    radConfig.num_samples = [_amdSamples integerValue];
    radConfig.ao_radius = [_amdSamples floatValue];
    
    radConfig.camera_focal_length = [_amdFocalLength floatValue];
    
    radConfig.camera_sensor_size.s[0] = [_amdSensorSize floatValue];
    radConfig.camera_sensor_size.s[1] = 2.0f * radConfig.camera_sensor_size.s[0] / 3.0f;
    
    
    radConfig.camera_sensor_size.s[1] = radConfig.height * radConfig.camera_sensor_size.s[0] / radConfig.width;
    
    float r_fov = [_generic_camera_fov floatValue] * M_PI / 180.0f;
    
    float dist = (radConfig.camera_sensor_size.s[1] / 2.0f) / (float)tan(r_fov / 2.0f);
    radConfig.camera_focal_length = dist;

    
    
    radConfig.camera_focus_distance = [_amdFocusDistance floatValue];
    radConfig.camera_aperture = [_amdAperture floatValue];
    switch ([_amdCameraType indexOfSelectedItem]) {
        case 0:
            radConfig.camera = mush::camera_type::perspective;
            break;
        case 1:
            radConfig.camera = mush::camera_type::perspective_dof;
            break;
        case 2:
            radConfig.camera = mush::camera_type::spherical_equirectangular;
            break;
    }
    
    radConfig.path = [[[_modelPath title] stringByDeletingLastPathComponent] UTF8String];
    radConfig.model_name = [[[_modelPath title] lastPathComponent] UTF8String];
    
    radConfig.environment_map_mult = [_amdEnvMapMult floatValue];
    radConfig.model_scale = [_genericModelScale floatValue];
    
    return radConfig;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

- (IBAction)overrideChecked:(id)sender {
    if ([_overrideSize state]) {
        [_overrideWidth setEnabled:YES];
        [_overrideHeight setEnabled:YES];
    } else {
        [_overrideWidth setEnabled:NO];
        [_overrideHeight setEnabled:NO];
    }
}
- (IBAction)plusClick:(id)sender {
}

- (IBAction)launch:(id)sender {
}
- (IBAction)processButton:(id)sender {
}

- (IBAction)processSelected:(id)sender {
}

- (IBAction)clearButton:(id)sender {
    unsigned length = [[_statsBox textStorage] length];
    [[_statsBox textStorage] replaceCharactersInRange:NSMakeRange(0, length)
                                      withString:@""];
}

- (IBAction)dualISOClicked:(id)sender {
    
    if ([_dualISO state]) {
        _firstISO.placeholderString = @"Scale";
        _secondISO.placeholderString = @"Frame Skip";
        _thirdISO.placeholderString = @"WB Red";
        _fourthISO.placeholderString = @"WB Blue";
    } else {
        _firstISO.placeholderString = @"First ISO";
        _secondISO.placeholderString = @"Second ISO";
        _thirdISO.placeholderString = @"Third ISO";
        _fourthISO.placeholderString = @"Fourth ISO";
    }
}

- (IBAction)livestream_checked:(id)sender {
    if ([_useHLS state]) {
        [_ffmpegPathButton setEnabled:FALSE];
        [_livestream_button setEnabled:TRUE];
    } else {
        [_ffmpegPathButton setEnabled:TRUE];
        [_livestream_button setEnabled:FALSE];
    }
}
- (IBAction)livestream_button_clicked:(id)sender {
    if (self.liveStreamWindow == nil) {
        self.liveStreamWindow = [[liveStreamWindowController alloc] initWithWindowNibName:@"liveStreamWindowController"];
        [self.liveStreamWindow showWindow:self];
    } else {
        [self.liveStreamWindow showWindow:self];
    }
}
- (IBAction)resize_checked:(id)sender {
    if ([_resizeCheck state]) {
        [_input_resize_width setEnabled:TRUE];
        [_input_resize_height setEnabled:TRUE];
    } else {
        [_input_resize_width setEnabled:FALSE];
        [_input_resize_height setEnabled:FALSE];
    }
}
- (IBAction)resetButtonClicked:(id)sender {
    videoMushUpdateCLKernels();
}
- (IBAction)data_button_clicked:(id)sender {
    [self.dataOutput showWindow:self];
}

- (BOOL)windowWillClose:(id)sender{
    [self.timer invalidate];
    [self.timer2 invalidate];
    [self.dataOutput close];
    self.dataOutput = nil;
    [self.mushQueueWindow close];
    self.mushQueueWindow = nil;
    [self.openglWindow close];
    self.openglWindow = nil;
    return TRUE;
}

- (IBAction)sceneRadioSelect:(id)sender {
    [[_userDefaults defaults] setInteger:[_sponza state] forKey:@"sponza_state"];
    [[_userDefaults defaults] setInteger:[_sibenik state] forKey:@"sibenik_state"];
    [[_userDefaults defaults] setInteger:[_office state] forKey:@"conf_state"];
    [[_userDefaults defaults] setInteger:[_kitchen state] forKey:@"kitchen_state"];
    
}


- (IBAction)parSettingsButtonClicked:(id)sender {
    if (self.parSettingWindow == nil) {
        self.parSettingWindow = [[parSettings alloc] initWithWindowNibName:@"parSettings"];
        [self.parSettingWindow showWindow:self];
    } else {
        [self.parSettingWindow showWindow:self];
    }
    
}

- (IBAction)queueButtonClicked:(id)sender {
    
    [self.mushQueueWindow showWindow:self];
    
    if (!_has_opened_queue) {
        _has_opened_queue = true;
        mushConfig * c = [[mushConfig alloc] initWithConfig:[self getConfig]];
        [_mushQueueWindow addRow:c];
    }
    
}
- (IBAction)nodeClick:(id)sender {
    
    if (self.openglWindow == nil) {
        self.openglWindow = [[openglWindowController alloc] initWithWindowNibName:@"openglWindowController"];
        [self.openglWindow showWindow:self];
    } else {
        [self.openglWindow showWindow:self];
    }
    
}

@end
