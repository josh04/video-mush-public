//
//  dataOutputWindow.h
//  video-mush
//
//  Created by Josh McNamee on 31/08/2016.
//
//

#import <Cocoa/Cocoa.h>
#include "ConfigStruct.hpp"

@interface dataOutputWindow : NSWindowController <NSTableViewDataSource, NSTableViewDelegate>
@property (weak) IBOutlet NSButton *save_button;
@property (weak) IBOutlet NSTableView *data_table;

- (void)addRow:(mush::metric_value *) arr array_size:(uint32_t)size;
- (void)setNames:(NSMutableArray *) name_strings;
@end
