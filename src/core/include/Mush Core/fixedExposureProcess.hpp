//
//  fixedExposureProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 13/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_fixedExposureProcess_hpp
#define media_encoder_fixedExposureProcess_hpp

#include "mush-core-dll.hpp"
#include "imageProcess.hpp"
#include "registerContainer.hpp"

namespace cl {
    class Kernel;
    class CommandQueue;
}

namespace mush {
    class MUSHEXPORTS_API fixedExposureProcess : public mush::imageProcess {
    public:
        fixedExposureProcess(float darken) : mush::imageProcess(), darken(darken) {
            
        }
        
        ~fixedExposureProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        void set(float x);
        
        void process() override;
        
    private:
        std::mutex _set_mutex;
        
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * exposure = nullptr;
        
        float darken = 0.0f;
        mush::registerContainer<mush::imageBuffer> buffer;
    };
        
}


#endif
