//
//  imageBuffer.hpp
//  video-mush
//
//  Created by Josh McNamee on 24/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_imageBuffer_hpp
#define video_mush_imageBuffer_hpp

#include "mush-core-dll.hpp"
#include "ringBuffer.hpp"
#include "guiAccessible.hpp"
#include "imageProperties.hpp"

class tagInGui;
namespace cl {
    class Image2D;
    class CommandQueue;
}

namespace mush {
	class MUSHEXPORTS_API imageBuffer : public guiAccessible {
    public:
        imageBuffer() : guiAccessible() {
            
        }
        
        ~imageBuffer() {
            
        }
        
        virtual void inUnlock();
        
    protected:
        
		cl_mem _getImageMem(uint8_t id) {
            return _getMem(id).get_image();
        }
        
    private:
    };
    
}

#endif
