//
//  mushQueue.h
//  video-mush
//
//  Created by Josh McNamee on 18/03/2017.
//
//

#import <Cocoa/Cocoa.h>
#include "ConfigStruct.hpp"
#import "mushConfig.h"

@interface mushQueue : NSWindowController <NSTableViewDataSource, NSTableViewDelegate>

@property (weak) IBOutlet NSButton *run_button;

@property (weak) IBOutlet NSTableView *queue_table;
@property (weak) IBOutlet NSTableView *done_table;


- (IBAction)runButtonPressed:(id)sender;
- (IBAction)addButtonPressed:(id)sender;
- (IBAction)removeButtonPressed:(id)sender;

- (void)addRow:(mushConfig *) config;

@end
