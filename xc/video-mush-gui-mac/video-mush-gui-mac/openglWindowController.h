//
//  openglWindowController.h
//  video-mush
//
//  Created by Josh McNamee on 03/10/2017.
//
//

#import <Cocoa/Cocoa.h>

@interface openglWindowController : NSWindowController

    @property (weak) IBOutlet NSOpenGLView *gl_view;
- (IBAction)gui_click:(id)sender;

@end
