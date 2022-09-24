//
//  openglWindowController.m
//  video-mush
//
//  Created by Josh McNamee on 03/10/2017.
//
//

#include <Mush Core/node_api.hpp>

#import "openglWindowController.h"

@interface openglWindowController ()

@end

@implementation openglWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.


}

- (void)update_gui {
    
    
    while (true) {
    }
}



- (IBAction)gui_click:(id)sender {
    
    
    node_create_gui();
    
    CGLContextObj cgl_context = CGLGetCurrentContext();
    
    [_gl_view setOpenGLContext:[[NSOpenGLContext alloc] initWithCGLContextObj:cgl_context]];
    
    
        node_update_gui();
    
}
@end
