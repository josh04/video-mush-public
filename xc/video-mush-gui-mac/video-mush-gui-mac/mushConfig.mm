//
//  mushConfig.m
//  video-mush
//
//  Created by Josh McNamee on 18/03/2017.
//
//

#import "mushConfig.h"

@implementation mushConfig

- (id)initWithConfig:(mush::config)config {
    self = [super init];
    if (self) {
        self.config = config;
    }
    return self;
}

@end
