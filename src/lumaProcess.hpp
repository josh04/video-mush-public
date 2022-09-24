//
//  lumaProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 10/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_lumaProcess_hpp
#define media_encoder_lumaProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class lumaProcess : public mush::imageProcess {
public:
    lumaProcess(float gamma, float darken) : mush::imageProcess(), gamma(gamma), darken(darken) {
        
    }
    
    ~lumaProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        luma = context->getKernel("luminance");
        
        weight.s[0] = 0.299f; weight.s[1] = 0.587f; weight.s[2] = 0.114f; weight.s[3] = 0.0f;
        
//        luma->setArg(1, *lumaImage);
        
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
        luma->setArg(0, input.get_image());
        luma->setArg(1, _getImageMem(0));
        luma->setArg(2, weight);
        luma->setArg(3, gamma);
        luma->setArg(4, darken);
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*luma, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        buffer->outUnlock();
        inUnlock();
    }

private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * luma = nullptr;
    float gamma = 0.4545f, darken = 0.0f;
    mush::registerContainer<mush::imageBuffer> buffer;
    cl_float4 weight;
};

#endif
