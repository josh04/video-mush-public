//
//  MushUpsampleController.h
//  video-mush
//
//  Created by Josh McNamee on 23/09/2015.
//
//

#import <Cocoa/Cocoa.h>

@interface MushUpsampleController : NSWindowController
@property (weak) IBOutlet NSTextField *gridCount;
@property (weak) IBOutlet NSTextField *geomWeight;
@property (weak) IBOutlet NSTextField *depthWeight;
@property (weak) IBOutlet NSTextField *kWeight;

@end
