//
//  liveStreamWindowController.h
//  video-mush
//
//  Created by Josh McNamee on 15/04/2016.
//
//

#import <Cocoa/Cocoa.h>

@interface liveStreamWindowController : NSWindowController
@property (weak) IBOutlet NSPopUpButton *webroot;
@property (weak) IBOutlet NSTextField *webaddress;
@property (weak) IBOutlet NSTextField *streamname;
@property (weak) IBOutlet NSTextField *num_files;
@property (weak) IBOutlet NSTextField *file_length;
@property (weak) IBOutlet NSTextField *wrap_after;
- (IBAction)webRootChoosePath:(id)sender;
@property (strong) IBOutlet NSUserDefaultsController *userDefaults;

@end
