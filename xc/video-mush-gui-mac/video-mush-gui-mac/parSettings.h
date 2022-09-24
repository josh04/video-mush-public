//
//  parSettings.h
//  video-mush
//
//  Created by Josh McNamee on 24/02/2017.
//
//

#import <Cocoa/Cocoa.h>
#include <ParFramework/parConfig.hpp>

@interface parSettings : NSWindowController
- (parConfigStruct)get_config;
@property (weak) IBOutlet NSButton *autoCoreLimit;
@property (weak) IBOutlet NSTextField *coreLimitBox;
@property (weak) IBOutlet NSTextField *openGLWidth;
@property (weak) IBOutlet NSTextField *openGLHeight;
@property (weak) IBOutlet NSTextField *cameraX;
@property (weak) IBOutlet NSTextField *cameraY;
@property (weak) IBOutlet NSTextField *cameraZ;
@property (weak) IBOutlet NSTextField *cameraTheta;
@property (weak) IBOutlet NSTextField *cameraPhi;
@property (weak) IBOutlet NSTextField *cameraFOV;
@property (weak) IBOutlet NSButton *setCameraPosition;
@property (weak) IBOutlet NSButton *secondCameraOffset;
@property (weak) IBOutlet NSButton *secondCameraJustMetadata;
@property (weak) IBOutlet NSButton *lowGraphicsMemoryMode;
@property (weak) IBOutlet NSButton *secondViewStereo;
@property (weak) IBOutlet NSTextField *secondCameraX;
@property (weak) IBOutlet NSTextField *secondCameraY;
@property (weak) IBOutlet NSTextField *secondCameraZ;
@property (weak) IBOutlet NSTextField *secondCameraTheta;
@property (weak) IBOutlet NSTextField *secondCameraPhi;
@property (weak) IBOutlet NSTextField *secondCameraFOV;
@property (weak) IBOutlet NSButton *parSaveFinal;
@property (weak) IBOutlet NSButton *parJustMetadata;

@end
