//
//  laplaceProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 04/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_laplaceProcess_hpp
#define video_mush_laplaceProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class laplaceProcess : public mush::imageProcess {
public:
    laplaceProcess() : mush::imageProcess() {
        
    }
    
    ~laplaceProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        laplace = context->getKernel("laplace");
        
        buffer = castToImage(buffers.begin()[0]);
        
        buffer->getParams(_width, _height, _size);
        
        addItem(context->floatImage(_width, _height));
        
        queue = context->getQueue();
    }
    
    void process() {
        inLock();
        auto input = buffer->outLock();
        if (input == nullptr) {
            release();
            return;
        }
        laplace->setArg(0, input.get_image());
        laplace->setArg(1, _getImageMem(0));
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*laplace, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        buffer->outUnlock();
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * laplace = nullptr;
    mush::registerContainer<mush::imageBuffer> buffer;
};

#endif
