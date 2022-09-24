//
//  rotateImage.hpp
//  video-mush
//
//  Created by Josh McNamee on 24/05/2015.
//
//

#ifndef video_mush_rotateImage_hpp
#define video_mush_rotateImage_hpp

#include "dll.hpp"

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class VIDEOMUSH_EXPORTS rotateImage : public mush::imageProcess {
    public:
        rotateImage() : mush::imageProcess() {
            
        }
        
        ~rotateImage() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
        
        void process();
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * rotate = nullptr;
        
        mush::registerContainer<mush::imageBuffer> buffer;
        
        uint32_t time = 0;
        
    };
    
}

#endif
