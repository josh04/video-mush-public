//
//  sbsPackProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 13/03/2017.
//
//

#include "sbsUnpackProcess.hpp"
#include <Mush Core/opencl.hpp>

sbsUnpackProcess::sbsUnpackProcess(bool is_right) : mush::imageProcess(), _is_right(is_right) {
    
}

sbsUnpackProcess::~sbsUnpackProcess() {
    
}

void sbsUnpackProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 1);
    
    _unpack = context->getKernel("sbs_unpack");
    
    _left = castToImage(buffers.begin()[0]);
    _left->getParams(_width, _height, _size);
    
    _width = _width / 2;
    
    addItem(context->floatImage(_width, _height));
    
    queue = context->getQueue();
}

void sbsUnpackProcess::process() {
    inLock();
    
    cl::Event event;
    
    auto dbl = _left->outLock();
    if (dbl == nullptr) {
        release();
        return;
    }
    
    _unpack->setArg(0, dbl.get_image());
    _unpack->setArg(1, _is_right);
    _unpack->setArg(2, _getImageMem(0));
    queue->enqueueNDRangeKernel(*_unpack, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    _left->outUnlock();
    inUnlock();
}
