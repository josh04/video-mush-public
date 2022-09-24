//
//  guiExposureProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/08/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_guiExposureProcess_hpp
#define video_mush_guiExposureProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainter.hpp>

class guiExposureProcess : public mush::imageProcess {
public:
    guiExposureProcess() : mush::imageProcess() {
        
    }
    
    ~guiExposureProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        exposure = context->getKernel("exposure");
        
        exposure->setArg(2, 0.0f);
        
        buffer = castToImage(buffers[0]);
        
        buffer->getParams(_width, _height, _size);
        
        addItem(context->floatImage(_width, _height));
        
        
        queue = context->getQueue();
    }
    
    void process() {
        if (tagGui != nullptr) {
            setExposure(tagGui->getExposure());
        }
        
        inLock();
        auto input = _buffer->outLock();
        if (input == nullptr) {
            release();
            return;
        }
        
        exposure->setArg(0, input.get_image());
        exposure->setArg(1, _getImageMem(0));
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*exposure, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        buffer->outUnlock();
        inUnlock();
    }
    
private:
    void setExposure(const float e) {
        exposure->setArg(2, e);
    }
    
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * exposure = nullptr;
    mush::registerContainer<mush::imageBuffer> buffer;
};

#endif
