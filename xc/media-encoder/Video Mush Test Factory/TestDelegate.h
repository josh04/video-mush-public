//
//  AppDelegate.h
//  Video Mush Test Factory
//
//  Created by Josh McNamee on 19/09/2015.
//
//

#import <Cocoa/Cocoa.h>

@interface TestDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate, NSTableViewDataSource>

@property (weak) IBOutlet NSWindow *window;
- (IBAction)plusClick:(id)sender;
- (IBAction)launch:(id)sender;
@property (unsafe_unretained) IBOutlet NSTextView *statsBox;
- (IBAction)processButton:(id)sender;
- (IBAction)processSelected:(id)sender;
@property (weak) IBOutlet NSTableView *inputRows;

@property (strong, nonatomic) MushUpsampleController * upsampleController;

@end

