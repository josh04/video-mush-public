//
//  dataOutputWindow.m
//  video-mush
//
//  Created by Josh McNamee on 31/08/2016.
//
//

#import "dataOutputWindow.h"
#import "ExportMenu.h"
#include "ConfigStruct.hpp"

@interface dataOutputWindow () {

    NSMutableArray * results_array;
    NSMutableArray * names_array;

    NSMutableDictionary * names_to_columns;
}

@end

@implementation dataOutputWindow

- (void)awakeFromNib {
    
    results_array = [[NSMutableArray alloc] init];
    names_to_columns = [[NSMutableDictionary alloc] init];
}

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
    
    if (names_array != nil) {
        [self setNames:names_array];
    }
    
    [_data_table setDelegate:self];
    [_data_table setDataSource:self];
    //results_array = [[NSMutableArray alloc] init];
    [_data_table setColumnAutoresizingStyle:NSTableViewSequentialColumnAutoresizingStyle];
    self.data_table.menu = [[ExportMenu alloc] initWithTableView:self.data_table];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return results_array.count;
}

- (NSView *)tableView:(NSTableView *)tableView
   viewForTableColumn:(NSTableColumn *)tableColumn
                  row:(NSInteger)row {
    
    // Get an existing cell with the MyView identifier if it exists
    NSTextField *result = [tableView makeViewWithIdentifier:@"MyView" owner:self];
    
    // There is no existing cell to reuse so create a new one
    if (result == nil) {
        
        // Create the new NSTextField with a frame of the {0,0} with the width of the table.
        // Note that the height of the frame is not really relevant, because the row height will modify the height.
        result = [[NSTextField alloc] init];
        
        // The identifier of the NSTextField instance is set to MyView.
        // This allows the cell to be reused.
        result.identifier = @"MyView";
    }
    
    // result is now guaranteed to be valid, either as a reused cell
    // or as a new cell, so set the stringValue of the cell to the
    // nameArray value at row
    //[tableColumn iden]
    
    [result setEditable:true];
    [result setBackgroundColor:[NSColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:0.0f]];
    
    
    NSNumber * ind = [names_to_columns objectForKey:[tableColumn title]];
    
    result.stringValue = [[results_array objectAtIndex:row] objectAtIndex:[ind integerValue]];
    
    [result sizeToFit];
    [result setBezeled:NO];

    // Return the result
    return result;
    
}

- (void)addRow:(mush::metric_value *) arr array_size:(uint32_t)size {
    
    
    NSMutableArray * values = [[NSMutableArray alloc] init];
    
    for (int i = 0; i < size; ++i) {
        //NSTextFieldCell * field = [[NSTextFieldCell alloc] init];
        NSString * val;
        if (arr[i].type == mush::metric_value_type::f) {
            val = [NSString stringWithFormat:@"%f", arr[i].value.floating_point];
        } else {
            val = [NSString stringWithFormat:@"%d", arr[i].value.integer];
        }
        //[field setStringValue:val];
        [values addObject:val];
    }
    
    [results_array addObject:values];
    if (_data_table != nil) {
        [_data_table reloadData];
    }
}

- (void)setNames:(NSMutableArray *) name_strings {
    
    names_array = [name_strings copy];
    
    if (_data_table != nil) {
    
    while ([[_data_table tableColumns] count] < [name_strings count]) {
        NSTableColumn * col = [[NSTableColumn alloc] init];
        [_data_table addTableColumn:col];
    }
    
    for (int i = 0; i < [name_strings count]; ++i) {
        NSTableColumn * col = [_data_table tableColumns][i];
        [col setTitle:[name_strings objectAtIndex:i]];
        
        NSString * str = [[name_strings objectAtIndex:i] copy];
        
        [names_to_columns setObject:[NSNumber numberWithInteger:i] forKey:str];
        
        //[col setResizingMask:NSTableColumnAutoresizingMask];
        CGFloat width = [col headerCell].cellSize.width;
        [col setMinWidth:width];
        [col setWidth:width];
    }
        
    }
}

@end
