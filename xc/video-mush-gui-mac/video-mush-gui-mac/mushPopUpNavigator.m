//
//  mushPopUpNagivator.m
//  video-mush
//
//  Created by Josh McNamee on 08/01/2017.
//
//

#import "mushPopUpNavigator.h"

@interface mushPopUpNavigator () {
    NSMutableArray *_saved_paths;
    
    NSUserDefaultsController * _defaults;
    NSString * _key;
    NSString * _append_key;
    
    bool _is_save;
    
}
@end

@implementation mushPopUpNavigator

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
    
    // Drawing code here.
}


- (IBAction)openButtonClicked:(id)sender {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    
    [openDlg setCanChooseFiles:_can_choose_files];
    [openDlg setCanChooseDirectories:_can_choose_directories];
    [openDlg setAllowsMultipleSelection:_can_choose_multiple];
    
    [self do_panel_action:openDlg];
}

- (IBAction)saveButtonClicked:(id)sender {
    NSSavePanel* saveDlg = [NSSavePanel savePanel];
    [saveDlg setCanCreateDirectories:YES];
    //NSString *ext = @"exr";
    
    [self do_panel_action:saveDlg];
}

- (void)do_panel_action:(NSSavePanel *) panel {
    
    if (_file_extension != nil) {
        [panel setAllowsOtherFileTypes:YES];
        [panel setAllowedFileTypes:@[ _file_extension ] ];
    }
    
    if ( [panel runModal] == NSFileHandlingPanelOKButton )
    {
        NSURL *url = [panel URL];
        if ([url isFileURL]) {
            NSString* fileName = [url path];
            [self addItemWithTitle: fileName];
            [self selectItemWithTitle: fileName];
            
            [_saved_paths addObject:fileName];
            if (_defaults != nil) {
                [[_defaults defaults] setObject:_saved_paths forKey:_key];
            }
        }
    } else {
        [self selectItemAtIndex:0];
    }
}

- (IBAction)selectionChanged:(id)sender {
    [[_defaults defaults] setInteger:[self indexOfSelectedItem] forKey:_append_key];
}

- (void)do_init {
    
    _can_choose_files = NO;
    _can_choose_directories = YES;
    _can_choose_multiple = NO;
    
    _saved_paths = [[NSMutableArray alloc] init];
    
    [self addItemWithTitle:@""];
    //[[self itemAtIndex:0] setState:1];
    //[[self itemAtIndex:0] setHidden:TRUE]; null option now
    [[self itemAtIndex:0] setEnabled:TRUE];
    
    [self addItemWithTitle:@"Choose path..."];
    
    [[self menu] addItem:[NSMenuItem separatorItem]];
    
    [[self itemAtIndex:1] setTarget:self];
    if (_is_save) {
        [[self itemAtIndex:1] setAction:@selector(saveButtonClicked:)];
    } else {
        [[self itemAtIndex:1] setAction:@selector(openButtonClicked:)];
    }
    [self setTarget:self];
    [self setAction:@selector(selectionChanged:)];

}
- (void)initAttachDefaults:(NSUserDefaultsController*) def withKey:(NSString *) key isSave:(bool) is_save
{
    _is_save = is_save;
    
    [self do_init];
    
    if (![[def defaults] arrayForKey:key]) {
        NSLog(@"no frame defaults");
        [[def defaults] setObject:_saved_paths forKey:key];
    } else {
        [_saved_paths addObjectsFromArray:[[def defaults] arrayForKey:key]] ;
    }
    
    NSOrderedSet *tmp1 = [NSOrderedSet orderedSetWithArray:_saved_paths];
    _saved_paths = [[tmp1 array] mutableCopy];
    
    if ([_saved_paths count] > 10) {
        _saved_paths = [[_saved_paths subarrayWithRange:NSMakeRange([_saved_paths count] - 10, 10)] mutableCopy];
    }
    
    [[def defaults] setObject:_saved_paths forKey:key];
    
    [self addItemsWithTitles:_saved_paths];
    
    NSString * append_key = [@"selected_" stringByAppendingString:key];
    _key = key;
    _append_key = append_key;
    [self selectItemAtIndex:[[def defaults] integerForKey:append_key]];
    
    _defaults = def;
}

@end
