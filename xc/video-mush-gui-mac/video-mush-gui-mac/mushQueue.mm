//
//  mushQueue.m
//  video-mush
//
//  Created by Josh McNamee on 18/03/2017.
//
//

#import "AppDelegate.h"
#import "mushQueue.h"
#import "ExportMenu.h"
#include "ConfigStruct.hpp"
#include <Mush Core/inputConfig.hpp>
#include <Mush Core/outputConfig.hpp>

@interface mushQueue () {
    
    NSMutableArray * queue_array;
    NSMutableArray * done_array;
    
    NSMutableArray * queue_names_array;
    NSMutableArray * done_names_array;
    
    NSMutableDictionary * names_to_columns;
    
    AppDelegate * main_window;
    
}
@end

@implementation mushQueue

- (void)awakeFromNib {
    queue_names_array = [[NSMutableArray alloc] init];
    
    [queue_names_array addObject:@"Queue"];
    [queue_names_array addObject:@"Input"];
    [queue_names_array addObject:@"Process"];
    [queue_names_array addObject:@"Output"];
    [queue_names_array addObject:@"Processing"];
    //[queue_names_array addObject:@"Transfer"];
    //[queue_names_array addObject:@"Encode"];
    
    done_names_array = [NSMutableArray arrayWithArray:queue_names_array];
    [done_names_array replaceObjectAtIndex:0 withObject:@"Done"];
    
    queue_array = [[NSMutableArray alloc] init];
    done_array = [[NSMutableArray alloc] init];
    names_to_columns = [[NSMutableDictionary alloc] init];
    
    [self setNames];
}

- (void)windowDidLoad {
    [super windowDidLoad];
    
    [_queue_table setDelegate:self];
    [_queue_table setDataSource:self];
    
    [_queue_table setColumnAutoresizingStyle:NSTableViewSequentialColumnAutoresizingStyle];
    
    
    [_done_table setDelegate:self];
    [_done_table setDataSource:self];
    
    [_done_table setColumnAutoresizingStyle:NSTableViewSequentialColumnAutoresizingStyle];
    
    
    
    //self.queue_table.menu = [[ExportMenu alloc] initWithTableView:self.queue_table];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    
    NSMutableArray * results_array = nil;
    if (tableView == _queue_table) {
        results_array = queue_array;
    } else {
        results_array = done_array;
    }
    
    
    return results_array.count;
}

- (NSView *)tableView:(NSTableView *)tableView
   viewForTableColumn:(NSTableColumn *)tableColumn
                  row:(NSInteger)row {
    
    NSTextField *result = [tableView makeViewWithIdentifier:@"MyView" owner:self];
    
    if (result == nil) {
        result = [[NSTextField alloc] init];
        result.identifier = @"MyView";
    }
    
    [result setEditable:true];
    [result setBackgroundColor:[NSColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:0.0f]];
    
    
    NSNumber * ind = [names_to_columns objectForKey:[tableColumn title]];
    
    NSMutableArray * results_array = nil;
    if (tableView == _queue_table) {
        results_array = queue_array;
    } else {
        results_array = done_array;
    }
    
    mushConfig * c = [results_array objectAtIndex:([results_array count] - row - 1)];
    
    long i = [ind integerValue];
    
    switch (i) {
        case 0:
        result.stringValue = [NSString stringWithFormat:@"%ld", row + 1];
        break;
        case 1:
        {
            const char * input = mush::inputEngineToString(c.config.inputConfig.inputEngine);
            result.stringValue = [NSString stringWithUTF8String:input];
        }
        break;
        case 2:
        
        {
            const char * pro = mush::processEngineToString(c.config.processEngine, c.config.genericChoice, c.config.generatorConfig.type);
            result.stringValue = [NSString stringWithUTF8String:pro];
        }
        
        break;
        case 3:
        {
            const char * output = mush::outputEngineToString(c.config.outputConfig.outputEngine);
            const char * trans = mush::transferToString(c.config.outputConfig.func);
            const char * enc = mush::encodeEngineToString(c.config.outputConfig.encodeEngine);
            if (c.config.outputConfig.outputEngine == mush::outputEngine::libavformatOutput) {
                result.stringValue = [NSString stringWithUTF8String:output];
                result.stringValue = [[result.stringValue stringByAppendingString:@" "]  stringByAppendingString:[NSString stringWithUTF8String:trans]];
                result.stringValue = [[result.stringValue stringByAppendingString:@" "]  stringByAppendingString:[NSString stringWithUTF8String:enc]];
            } else {
                
                result.stringValue = [NSString stringWithUTF8String:enc];
                if (c.config.outputConfig.encodeEngine != mush::encodeEngine::none) {
                    result.stringValue = [[result.stringValue stringByAppendingString:@" "]  stringByAppendingString:[NSString stringWithUTF8String:trans]];
                }
            }
        }
        break;
        default:
        result.stringValue = @"";
        break;
        
    }
    
    [result sizeToFit];
    [result setBezeled:NO];
    
    return result;
    
}

- (IBAction)runButtonPressed:(id)sender {
    
    while ([queue_array count]) {
        mushConfig * c = [queue_array lastObject];
        
        bool success = [main_window runConfig:c.config];
        
        if (!success) {
            break;
        }
        
        [queue_array removeLastObject];
        [_queue_table reloadData];
        [_queue_table sizeToFit];
        [self doneRow:c];
    }
    
}

- (IBAction)addButtonPressed:(id)sender {
    
    mushConfig * c = [[mushConfig alloc] initWithConfig:[main_window getConfig]];
    [self addRow:c];
    
}

- (IBAction)removeButtonPressed:(id)sender {
    NSUInteger i = [queue_array count] - [_queue_table selectedRow] - 1;
    if (i >= 0 && i < [queue_array count]) {
        [queue_array removeObjectAtIndex:i];
        [_queue_table reloadData];
        [_queue_table sizeToFit];
    }
}

- (void)addRow:(mushConfig *) config {
    
    [queue_array addObject:config];
    if (_queue_table != nil) {
        [_queue_table reloadData];
        [_queue_table sizeToFit];
    }
    
}

- (void)doneRow:(mushConfig *) config {
    
    [done_array addObject:config];
    if (_done_table != nil) {
        [_done_table reloadData];
        [_done_table sizeToFit];
    }
    
}

- (void)setNames {
    
    if (_queue_table != nil) {
        
        while ([[_queue_table tableColumns] count] < [queue_names_array count]) {
            NSTableColumn * col = [[NSTableColumn alloc] init];
            [_queue_table addTableColumn:col];
        }
        
        for (int i = 0; i < [queue_names_array count]; ++i) {
            NSTableColumn * col = [_queue_table tableColumns][i];
            [col setTitle:[queue_names_array objectAtIndex:i]];
            
            NSString * str = [[queue_names_array objectAtIndex:i] copy];
            
            [names_to_columns setObject:[NSNumber numberWithInteger:i] forKey:str];
            
            //[col setResizingMask:NSTableColumnAutoresizingMask];
            CGFloat width = [col headerCell].cellSize.width;
            [col setMinWidth:width];
            [col setWidth:width];
        }
        
    }
    
    if (_done_table != nil) {
        
        while ([[_done_table tableColumns] count] < [done_names_array count]) {
            NSTableColumn * col = [[NSTableColumn alloc] init];
            [_done_table addTableColumn:col];
        }
        
        for (int i = 0; i < [done_names_array count]; ++i) {
            NSTableColumn * col = [_done_table tableColumns][i];
            [col setTitle:[done_names_array objectAtIndex:i]];
            
            NSString * str = [[done_names_array objectAtIndex:i] copy];
            
            [names_to_columns setObject:[NSNumber numberWithInteger:i] forKey:str];
            
            //[col setResizingMask:NSTableColumnAutoresizingMask];
            CGFloat width = [col headerCell].cellSize.width;
            [col setMinWidth:width];
            [col setWidth:width];
        }
        
    }
}

- (void)showWindow:(id)sender {
    [super showWindow:sender];
    
    main_window = sender;
    [main_window.encodeButton setEnabled:FALSE];
    
}

- (void)windowWillClose:(id)sender {
    [main_window.encodeButton setEnabled:TRUE];
}

@end
