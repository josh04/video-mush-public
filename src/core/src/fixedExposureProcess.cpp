//
//  fixedExposureProcess.cpp
//  mush-core
//
//  Created by Josh McNamee on 11/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#include "opencl.hpp"
#include "fixedExposureProcess.hpp"

namespace mush {
    void fixedExposureProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        exposure = context->getKernel("exposure");
        
        buffer = castToImage(buffers.begin()[0]);
        
        buffer->getParams(_width, _height, _size);
        
        addItem(context->floatImage(_width, _height));
        
        
        queue = context->getQueue();
    }
    
    void fixedExposureProcess::set(float x) {
        //std::lock_guard<std::mutex> lock(_set_mutex);
        darken = x;
    }
    
    void fixedExposureProcess::process() {
        auto buf = inLock();
        auto input = buffer->outLock();
        if (input == nullptr) {
            release();
            return;
        }

		buf.copy_parameters(input);

        exposure->setArg(0, input.get_image());
        exposure->setArg(1, _getImageMem(0));
        //{
            //std::lock_guard<std::mutex> lock(_set_mutex);
            exposure->setArg(2, darken);
        //}
        cl::Event event;
        queue->enqueueNDRangeKernel(*exposure, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        buffer->outUnlock();
        inUnlock();
    }
}
