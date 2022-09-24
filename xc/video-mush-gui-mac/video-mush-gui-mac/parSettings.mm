//
//  parSettings.m
//  video-mush
//
//  Created by Josh McNamee on 24/02/2017.
//
//

#include <stdint.h>

#import "parSettings.h"

@interface parSettings ()

@end

@implementation parSettings

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}

- (parConfigStruct)get_config {
    parConfigStruct par_config;
    par_config.defaults();
    par_config.auto_cores = [_autoCoreLimit state];
    par_config.manual_cores = [_coreLimitBox integerValue];
    par_config.gl_width = [_openGLWidth integerValue];
    par_config.gl_height = [_openGLHeight integerValue];
    
    /*
    par_config.camera_x = [_cameraX floatValue];
    par_config.camera_y = [_cameraY floatValue];
    par_config.camera_z = [_cameraZ floatValue];
    par_config.camera_theta = [_cameraTheta floatValue];
    par_config.camera_phi = [_cameraPhi floatValue];
    */
    par_config.place_camera = [_setCameraPosition state];
    par_config.second_view = [_secondCameraOffset state];
    par_config.second_view_just_metadata = [_secondCameraJustMetadata state];
    par_config.low_graphics_memory = [_lowGraphicsMemoryMode state];
    par_config.second_view_stereo = [_secondViewStereo state];
    
    par_config.second_camera_x_diff = [_secondCameraX floatValue];
    par_config.second_camera_y_diff = [_secondCameraY floatValue];
    par_config.second_camera_z_diff = [_secondCameraZ floatValue];
    par_config.second_camera_theta_diff = [_secondCameraTheta floatValue];
    par_config.second_camera_phi_diff = [_secondCameraPhi floatValue];
    par_config.second_camera_fov_diff = [_secondCameraFOV floatValue];
    par_config.final_snapshot = [_parSaveFinal state];
    par_config.just_get_metadata = [_parJustMetadata state];
    
    return par_config;
}

@end
