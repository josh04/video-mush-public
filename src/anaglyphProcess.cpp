//
//  anaglyphProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 19/02/2017.
//
//

#include "anaglyphProcess.hpp"

#include <Mush Core/opencl.hpp>



anaglyphProcess::anaglyphProcess() : mush::imageProcess() {
    
}

anaglyphProcess::~anaglyphProcess() {
    
}

void anaglyphProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 2);
    
    _anaglyph = context->getKernel("anaglyph");
    
    _left = castToImage(buffers.begin()[0]);
    _left->getParams(_width, _height, _size);
    
    _right = castToImage(buffers.begin()[1]);
    
    addItem(context->floatImage(_width, _height));
    
    queue = context->getQueue();
}

void anaglyphProcess::process() {
    inLock();
    
    cl::Event event;
    
    auto left = _left->outLock();
    if (left == nullptr) {
        release();
        return;
    }
    
    auto right = _right->outLock();
    if (right == nullptr) {
        release();
        return;
    }
    
    _anaglyph->setArg(0, left.get_image());
    _anaglyph->setArg(1, right.get_image());
    _anaglyph->setArg(2, _getImageMem(0));
    _anaglyph->setArg(3, _mode);
    queue->enqueueNDRangeKernel(*_anaglyph, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    _left->outUnlock();
    _right->outUnlock();
    inUnlock();
}
