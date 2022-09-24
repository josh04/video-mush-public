//
//  liveStreamWindowController.m
//  video-mush
//
//  Created by Josh McNamee on 15/04/2016.
//
//

#import "liveStreamWindowController.h"

@interface liveStreamWindowController () {

NSMutableArray *_webroots;

}
@end

@implementation liveStreamWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    _webroots = [[NSMutableArray alloc] init];
    
    if (![[_userDefaults defaults]	arrayForKey:@"webRoots"]) {
        NSLog(@"no webroot defaults");
        [[_userDefaults defaults] setObject:_webroots forKey:@"webRoots"];
    } else {
        [_webroots addObjectsFromArray:[[_userDefaults defaults] arrayForKey:@"webRoots"]] ;
    }
    
    NSOrderedSet *tmp1 = [NSOrderedSet orderedSetWithArray:_webroots];
    _webroots = [[tmp1 array] mutableCopy];
    
    if ([_webroots count] > 10) {
        _webroots = [[_webroots subarrayWithRange:NSMakeRange([_webroots count] - 10, 10)] mutableCopy];
    }
    
    
    [[_userDefaults defaults] setObject:_webroots forKey:@"webRoots"];
    
    [_webroot addItemsWithTitles:_webroots];
}

- (IBAction)webRootChoosePath:(id)sender {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    [openDlg setCanChooseFiles:NO];
    [openDlg setCanChooseDirectories:YES];
    [openDlg setAllowsMultipleSelection:NO];
    if ( [openDlg runModal] == NSFileHandlingPanelOKButton )
    {
        NSURL *url = [openDlg URL];
        if ([url isFileURL]) {
            NSString* fileName = [url path];
            [_webroot addItemWithTitle: fileName];
            [_webroot selectItemWithTitle: fileName];
            
            
            [_webroots addObject:fileName];
            [[_userDefaults defaults] setObject:_webroots forKey:@"webRoots"];
        }
    } else {
        [_webroot selectItemAtIndex:0];
    }
}

@end
