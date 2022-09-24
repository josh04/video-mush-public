//
//  dropFrameProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 04/11/2016.
//
//

#include "dropFrameProcess.hpp"

namespace mush {
    dropFrameProcess::dropFrameProcess(uint32_t interval) : mush::imageProcess(), _interval(interval) {
        
    }
    
    dropFrameProcess::~dropFrameProcess() {
        
    }
    
    void dropFrameProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer> > buffers) {
        
        assert(buffers.size() == 0);
        
        _input = castToImage(buffers.begin()[0]);
        
        _input->getParams(_width, _height, _size);
        
        addItem(context->floatImage(_width, _height));
        
        queue = context->getQueue();
    }
    
    void dropFrameProcess::process() {
        
        auto input = _input->outLock();
        if (input == nullptr) {
            release();
            return;
        }
        
        if (_tick % _interval == 0) {
            _tick = 0;
        
            inLock();
            
            _copy->setArg(0, input.get_image());
            _copy->setArg(1, _getImageMem(0));
            
            cl::Event event;
            queue->enqueueNDRangeKernel(*_copy, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            inUnlock();
        }
        
        _input->outUnlock();
        
        
        _tick++;
    }
    
}
