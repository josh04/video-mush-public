//
//  tonemapProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 11/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_tonemapProcess_hpp
#define media_encoder_tonemapProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class tonemapProcess : public mush::imageProcess {
public:
    tonemapProcess() : mush::imageProcess() {
        
    }
    
    ~tonemapProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        tonemap = context->getKernel("tonemap");
        
        buffer = castToImage(buffers.begin()[0]);
        
        buffer->getParams(_width, _height, _size);
        
        
        //addItem((unsigned char *)context->intBGRAImage(_width, _height));
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
        tonemap->setArg(0, input.get_image());
        tonemap->setArg(1, _getImageMem(0));
        if (_hack_flag) {
            tonemap->setArg(2, 1);
        } else {
            tonemap->setArg(2, 0);
        }
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*tonemap, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        buffer->outUnlock();
        inUnlock();
    }
    
    void hack_flag() {
        _hack_flag = true;
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * tonemap = nullptr;
    mush::registerContainer<mush::imageBuffer> buffer;
    float _gamma;
    
    bool _hack_flag = false;
    
};

#endif
