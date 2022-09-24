//
//  imageProperties.cpp
//  video-mush
//
//  Created by Josh McNamee on 02/02/2015.
//
//

#include "imageProperties.hpp"

void mush::imageProperties::getParams(unsigned int &width, unsigned int &height, unsigned int &lengthInFrames) {
    width = _width;
    height = _height;
    lengthInFrames = _size;
}
