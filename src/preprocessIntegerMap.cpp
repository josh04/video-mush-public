//
//  preprocessIntegerMap.cpp
//  video-mush
//
//  Created by Josh McNamee on 23/09/2015.
//
//

#include <memory>
#include <Mush Core/opencl.hpp>
#include "preprocessIntegerMap.hpp"
#include <Mush Core/frameGrabber.hpp>

namespace mush {

void preprocessIntegerMap::init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::frameGrabber> input, bool toGrayscale) {
    _queue = context->getQueue();
    _buffer = input;
    _to_grayscale = toGrayscale;
    setTagInGuiName(input->getTagInGuiName());
    
    _buffer->getParams(_width, _height, _size);
    if (toGrayscale) {
        _conversion = context->getKernel("charRGBtoGrayscale");
        _temp = context->buffer(_width * _height * 3 * sizeof(unsigned char));
        _conversion->setArg(0, *_temp);
        addItem(context->buffer(_width * _height * 1 * sizeof(unsigned char)));
        
    } else {
        addItem(context->buffer(_width * _height * 3 * sizeof(unsigned char)));
    }
}

void preprocessIntegerMap::process() {
    auto buf = _buffer->outLock();
    if (buf == nullptr) {
        release();
        return;
    }
	unsigned char * in_ptr = (unsigned char *)buf.get_pointer();
    if (_to_grayscale) {
        
        cl::Event event;
        _queue->enqueueWriteBuffer(*_temp, CL_TRUE, 0, _width*_height*3*sizeof(unsigned char), in_ptr, NULL, &event);
        event.wait();
        
        auto out_ptr = inLock();
        if (out_ptr == nullptr) {
            release();
            return;
        }
        _conversion->setArg(1, out_ptr.get_buffer());
        
        queue->enqueueNDRangeKernel(*_conversion, cl::NullRange, cl::NDRange(_width*_height, 1), cl::NullRange, NULL, &event);
        event.wait();
        
        inUnlock();
    } else {
        auto out_ptr = inLock();
        if (out_ptr == nullptr) {
            release();
            return;
        }
        
        cl::Event event;
        _queue->enqueueWriteBuffer(out_ptr.get_buffer(), CL_TRUE, 0, _width*_height*3*sizeof(unsigned char), in_ptr, NULL, &event);
        event.wait();
        
        inUnlock();
    }
        
    _buffer->outUnlock();
}

void preprocessIntegerMap::gather() {
    while (good()) {
        process();
    }
    release();
}

}