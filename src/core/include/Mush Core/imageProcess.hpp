//
//  imageProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 10/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_imageProcess_hpp
#define media_encoder_imageProcess_hpp

#include <assert.h>
#include "mush-core-dll.hpp"
#include "imageBuffer.hpp"
#include "processNode.hpp"
#include "initNode.hpp"

namespace mush {
    
    MUSHEXPORTS_API std::shared_ptr<mush::imageBuffer> castToImage(const std::shared_ptr<mush::ringBuffer> in);
    
    class imageProcess : public imageBuffer, public processNode, public initNode {
    public:
        imageProcess() : imageBuffer() {
            
        }
        
        ~imageProcess() {
            
        }
        
    private:
        
    };
    
}

#endif
