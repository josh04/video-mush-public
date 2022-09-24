//
//  main.m
//  factory
//
//  Created by Josh McNamee on 24/03/2015.
//
//

#import <Cocoa/Cocoa.h>

#include <Video Mush/exports.hpp>
#include "factoryProcessor.hpp"

int main(int argc, const char * argv[]) {
    mush::config config;
    config.defaults();
    
    // vlc output
    config.inputEngine = mush::inputEngine::noInput;
    config.outputConfig.encodeEngine = mush::encodeEngine::none;
    config.outputEngine = mush::outputEngine::noOutput;
    
    config.show_gui = true;
    // Sim2 preview window
    config.sim2preview = false;
    
    NSString * frDir = [NSString stringWithFormat:@"%@/%@", [[NSBundle mainBundle] bundlePath], @"Contents/Frameworks/Video Mush.framework"];
    NSString * resDir = [[[NSBundle bundleWithPath:frDir] resourcePath] stringByAppendingString:@"/"];
    config.resourceDir = [resDir UTF8String];
    config.inputConfig.resourceDir = [resDir UTF8String];
    /*
     config.resourceDir = ".\\";
     config.inputConfig.resourceDir = ".\\resources\\";
     */
    auto hg = std::make_shared<factory::processor>([[[NSBundle mainBundle] resourcePath] UTF8String]);
    
    videoMushInit(&config);
    videoMushExecute({}, hg);

}
