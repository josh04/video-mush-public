//
//  AppDelegate.m
//  Video Mush Test Factory
//
//  Created by Josh McNamee on 19/09/2015.
//
//
#include <memory>
#include <vector>

#include <Video Mush/CL/cl.hpp>
#define _MUSH_OPENCL_FRAMEWORK
#include <Mush Core/opencl.hpp>
#include <Video Mush/tagInGui.hpp>
#include <Mush Core/opencl.hpp>
#include <Video Mush/singleEXRInput.hpp>
#include <Video Mush/singleTIFFInput.hpp>
#include <Video Mush/mushPreprocessor.hpp>
#include <Video Mush/preprocessIntegerMap.hpp>

#include <Video Mush/rotateImage.hpp>
#include <Mush Core/opencl.hpp>
#include <Video Mush/trayraceUpsample.hpp>

#include <Video Mush/ConfigStruct.hpp>
#include "exports.hpp"

#import "MushUpsampleController.h"
#import "TestDelegate.h"

@interface TestDelegate () {
    std::shared_ptr<tagInGui> tagGui;
    std::shared_ptr<mush::opencl> context;
    
    std::vector<std::shared_ptr<mush::frameGrabber>> inputs;
    std::vector<std::shared_ptr<mush::ringBuffer>> preprocesses;
    
    std::shared_ptr<mush::imageProcess> process;
    
    NSString * resources;
    std::string resources_utf8;
    
    std::vector<std::thread *> preprocess_threads;
    
    NSTimer * log_timer;
    NSTimer * gui_timer;
    
    std::atomic_bool finished;
    BOOL process_initialised;
    
    NSMutableArray* inputRowsArray;
}

@end

@implementation TestDelegate

- (TestDelegate*)init {
    inputRowsArray = [[NSMutableArray alloc] init];
    self = [super init];
    tagGui = nullptr;
    context = nullptr;
    process_initialised = false;
    
    dispatch_block_t log_block = ^{
        self->log_timer = [NSTimer timerWithTimeInterval:0.05 target:self selector:@selector(logStack) userInfo:nil repeats:YES];
        [[NSRunLoop mainRunLoop] addTimer:self->log_timer forMode:NSDefaultRunLoopMode];
    };
    
    dispatch_async(dispatch_get_main_queue(), log_block);
    
    NSString * frDir = [NSString stringWithFormat:@"%@/%@", [[NSBundle mainBundle] bundlePath], @"Contents/Frameworks/Video Mush.framework"];
    NSString * resDir = [[[NSBundle bundleWithPath:frDir] resourcePath] stringByAppendingString:@"/"];
    
    resources = [resDir copy];
    resources_utf8 = [resources UTF8String];
   
    
    tagGui = std::make_shared<tagInGui>();
    tagGui->createGLContext(false, resources_utf8.c_str());
    context = tagGui->createInteropContext(resources_utf8.c_str(), false);
    
    std::vector<std::shared_ptr<mush::guiAccessible>> buffers = {};
    tagGui->init(context, buffers, "/Users/josh04/Pictures/EXR", 8);
    
    
    tagGui->update();
    
    
    return self;
    
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
}

- (void)awakeFromNib {
    [_inputRows setDataSource:self];
    
    dispatch_block_t gui_block = ^{

        self->gui_timer = [NSTimer timerWithTimeInterval:0.016 target:self selector:@selector(updateGui) userInfo:nil repeats:YES];
        [[NSRunLoop mainRunLoop] addTimer:self->gui_timer forMode:NSDefaultRunLoopMode];
    };
    
    dispatch_queue_t queue = dispatch_get_main_queue();
    
    dispatch_async(queue, gui_block);
    
    [_window setDelegate:self];
    [_window makeKeyAndOrderFront:self];
    
    finished = false;
    auto localTagGui = tagGui;
    while (!finished) {
        if (localTagGui != nullptr) {
            localTagGui->processEvents();
            usleep(2000);
        }
    }
    
}

- (IBAction)plusClick:(id)sender {
    
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles:YES];
    [openDlg setCanChooseDirectories:NO];
    [openDlg setAllowsMultipleSelection:YES];
    if ( [openDlg runModal] == NSFileHandlingPanelOKButton )
    {
        for (NSURL *url in [openDlg URLs]) {
            if ([url isFileURL]) {
                NSString* fileName = [url path];
                if ([[url pathExtension] isEqual: @"exr"]) {
                    [self addSingleEXRInput:fileName];
                } else if ([[url pathExtension] isEqual: @"tiff"] || [[url pathExtension] isEqual: @"tif"]) {
                    [self addSingleTIFFInput:fileName];
                }
            }
        }
    }
}

- (void)addSingleEXRInput:(NSString*)path {
    auto input = std::make_shared<mush::singleEXRInput>();
    mush::core::inputConfigStruct config;
    config.defaults();
    config.inputPath = [path UTF8String];
    input->init(context, config);
    
    inputs.push_back(input);
    
    auto preprocess = std::make_shared<mush::mushPreprocessor>();
    preprocess->init(context, input, mush::filetype::detectFiletype, (mush::input_pix_fmt)0, 2, 0.0, 1.0, 0, [resources UTF8String], config.func);
    
    tagGui->addSubScreen(preprocess);
    
    preprocesses.push_back(preprocess);
    
    input->gather();
    preprocess_threads.push_back(new std::thread(&mush::mushPreprocessor::preprocess, preprocess));
    
    [inputRowsArray addObject:path];
    [_inputRows reloadData];
}

- (void)addSingleTIFFInput:(NSString*)path {
    auto input = std::make_shared<mush::singleTIFFInput>();
    mush::core::inputConfigStruct config;
    config.defaults();
    config.inputPath = [path UTF8String];
    input->init(context, config);
    
    inputs.push_back(input);
    
    auto preprocess = std::make_shared<mush::preprocessIntegerMap>();
    preprocess->init(context, input, true);
    
    tagGui->addSubScreen(preprocess);
    
    preprocesses.push_back(preprocess);
    
    input->gather();
    preprocess_threads.push_back(new std::thread(&mush::preprocessIntegerMap::gather, preprocess));
    
    [inputRowsArray addObject:path];
    [_inputRows reloadData];
}

- (IBAction)processButton:(id)sender {
    if (process != nullptr && process_initialised) {
        context->makeProgram();
        if (self.upsampleController != nil) {
            auto upsample = std::dynamic_pointer_cast<trayraceUpsample>(process);
            
            upsample->setParams([[self.upsampleController gridCount] intValue], [[self.upsampleController geomWeight] doubleValue], [[self.upsampleController depthWeight] doubleValue], [[self.upsampleController kWeight] doubleValue]);
        }
        process->process();
    } else {
        putLog("Process not initialised.");
    }
}

- (void)updateGui {
    if (tagGui != nullptr) {
        //int i = tagGui->processEvents();
        tagGui->update();
    }
}

- (void)logStack {
    char logs[4096];
    bool done = getLog(logs, 4096);
    if (done) {
        NSAttributedString* attr = [[NSAttributedString alloc] initWithString:
                                    [NSString stringWithFormat: @"%s\n", logs]];
        
        
        [[_statsBox textStorage] appendAttributedString:attr];
        [_statsBox scrollRangeToVisible:NSMakeRange([[_statsBox string] length], 0)];
        
        [_statsBox setEditable:YES];
        [_statsBox setFont:[NSFont userFixedPitchFontOfSize:9.0]];
        [_statsBox setEditable:NO];
    }
}


- (void)windowWillClose:(NSNotification *)notification {
    finished = true;
    if (tagGui != nullptr) {
        tagGui->fireQuitEvent();
        tagGui->processEvents();
    }
    //usleep(2000); // oh dear...
    for (auto preprocess : preprocesses) {
        if (preprocess != nullptr) {
            preprocess->kill();
        }
    }
    for (auto preprocess_thread : preprocess_threads){
        if (preprocess_thread != nullptr) {
            if (preprocess_thread->joinable()) {
                preprocess_thread->join();
            }
        }
    }
    inputs.clear();
    preprocesses.clear();
    
    tagGui = nullptr;
    context = nullptr;
}

- (void)processSelected:(id)sender {
    NSPopUpButton* processBox = (NSPopUpButton*)sender;
    self.upsampleController = nil;
    process_initialised = false;
    switch ([processBox indexOfSelectedItem]) {
        case 2:
            process = std::make_shared<mush::rotateImage>();
            if (preprocesses.size() < 1) {
                putLog("Rotate Image Process did not initialise: Not enough inputs.");
            } else {
                try {
                    process->init(context, {preprocesses[0]});
                    process->setTagInGuiName("Rotate Image Test");
                    process_initialised = true;
                    putLog("Rotate Image Process initialised.");
                } catch (std::exception &e) {
                    putLog(e.what());
                    putLog("Rotate Image Process did not initialise: Bad inputs.");
                }
            }
            break;
        case 3:
            process = std::make_shared<trayraceUpsample>();
            if (preprocesses.size() < 5) {
                putLog("Upsample Process did not initialise: Not enough inputs.");
            } else {
                try {
                    process->init(context, {preprocesses[0], preprocesses[1], preprocesses[2], preprocesses[3], preprocesses[4]});
                    process->setTagInGuiName("Spatial Upsample Test");
                    process_initialised = true;
                    putLog("Upsample Process initialised.");
                    
                    self.upsampleController = [[MushUpsampleController alloc] initWithWindowNibName:@"MushUpsampleController"];
                    [self.upsampleController showWindow:self];
                    
                    [self.upsampleController gridCount].stringValue = @"0";
                    [self.upsampleController geomWeight].stringValue = @"0.0004";
                    [self.upsampleController depthWeight].stringValue = @"0.0001";
                    [self.upsampleController kWeight].stringValue = @"10.0";
                } catch (std::exception &e) {
                    putLog(e.what());
                    putLog("Upsample Process did not initialise: Bad inputs.");
                }
            }
            break;
        case 4:
            process = std::make_shared<mush::rotateImage>();
            break;
    }
    
    if (process_initialised) {
        tagGui->addSubScreen(process);
    }
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}

- (NSInteger) numberOfRowsInTableView:(NSTableView *)aTableView {
    return (int)[inputRowsArray count];
}

- (id) tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    
    id returnValue=nil;
    
    returnValue = [inputRowsArray objectAtIndex:rowIndex];
    
    return returnValue;
}



- (IBAction)clearButton:(id)sender {
}
@end
