//
//  imageProperties.hpp
//  video-mush
//
//  Created by Josh McNamee on 02/02/2015.
//
//

#ifndef video_mush_imageProperties_hpp
#define video_mush_imageProperties_hpp

#include "mush-core-dll.hpp"

namespace mush {
	class MUSHEXPORTS_API imageProperties {
    public:
		virtual void getParams(unsigned int &width, unsigned int &height, unsigned int &lengthInFrames);
    protected:
        unsigned int _width = 1280, _height = 720, _size = 1;
    };
}

#endif
