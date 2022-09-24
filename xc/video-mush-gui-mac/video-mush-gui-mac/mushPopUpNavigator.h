//
//  mushPopUpNagivator.h
//  video-mush
//
//  Created by Josh McNamee on 08/01/2017.
//
//

#import <Cocoa/Cocoa.h>

@interface mushPopUpNavigator : NSPopUpButton


@property (nonatomic, setter=setCanChooseFiles:) bool can_choose_files;
@property (nonatomic, setter=setCanChooseDirectories:) bool can_choose_directories;
@property (nonatomic, setter=setCanChooseMultiple:) bool can_choose_multiple;
@property (nonatomic, strong, setter=setFileExtension:) NSString * file_extension;

- (IBAction)saveButtonClicked:(id)sender;
- (IBAction)openButtonClicked:(id)sender;

- (void)initAttachDefaults:(NSUserDefaultsController*) def withKey:(NSString *) key isSave:(bool) is_save;

@end
