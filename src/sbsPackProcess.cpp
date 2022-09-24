//
//  sbsPackProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 13/03/2017.
//
//

#include "sbsPackProcess.hpp"
#include <Mush Core/opencl.hpp>

sbsPackProcess::sbsPackProcess() : mush::imageProcess() {
    
}

sbsPackProcess::~sbsPackProcess() {
    
}

void sbsPackProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 2);
    
	setTagInGuiStereo(true);

    _pack = context->getKernel("sbs_pack");
    
    _left = castToImage(buffers.begin()[0]);
    _left->getParams(_width, _height, _size);
    
    _width = _width * 2;
    
    _right = castToImage(buffers.begin()[1]);
    
    
    addItem(context->floatImage(_width, _height));
    
    queue = context->getQueue();
}

void sbsPackProcess::process() {
    inLock();
    
    cl::Event event;
    
    auto l = _left->outLock();
    if (l == nullptr) {
        release();
        return;
    }
    
    auto r = _right->outLock();
    if (r == nullptr) {
        release();
        return;
    }
    
    _pack->setArg(0, l.get_image());
    _pack->setArg(1, r.get_image());
    _pack->setArg(2, _getImageMem(0));
    queue->enqueueNDRangeKernel(*_pack, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    _right->outUnlock();
    _left->outUnlock();
    inUnlock();
}
