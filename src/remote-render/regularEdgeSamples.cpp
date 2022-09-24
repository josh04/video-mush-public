//
//  regularEdgeSamples.cpp
//  video-mush
//
//  Created by Josh McNamee on 26/10/2016.
//
//

#include <Mush Core/opencl.hpp>

#include "regularEdgeSamples.hpp"


regularEdgeSamples::regularEdgeSamples() : mush::imageProcess() {
    
}

regularEdgeSamples::~regularEdgeSamples() {
    
}

void regularEdgeSamples::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 1);
    
    samples = context->getKernel("edge_samples");
    clear = context->getKernel("edge_clear");
    
    imageBuffe = castToImage(buffers.begin()[0]);
    imageBuffe->getParams(_width, _height, _size);
    _width_full = _width;
    _height_full = _height;
    _width = (_width - (_width % 32))/32;
    _height = (_height - (_height %32))/32;
    
    //addItem(context->floatImage(_width, _height));
    addItem((unsigned char *)context->greyImage(_width, _height));
    
    queue = context->getQueue();
}

void regularEdgeSamples::process() {
    inLock();
    
    clear->setArg(0, _getImageMem(0));
    cl::Event event;
    queue->enqueueNDRangeKernel(*clear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    auto image = imageBuffe->outLock();
    if (image == nullptr) {
        release();
        return;
    }
    
    samples->setArg(0, image.get_image());
    samples->setArg(1, _getImageMem(0));
    queue->enqueueNDRangeKernel(*samples, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    imageBuffe->outUnlock();
    inUnlock();
}
