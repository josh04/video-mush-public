//
//  mushConfig.h
//  video-mush
//
//  Created by Josh McNamee on 18/03/2017.
//
//

#include "ConfigStruct.hpp"
#import <Foundation/Foundation.h>

@interface mushConfig : NSObject

- (id)initWithConfig:(mush::config)config;

@property mush::config config;

@end
