//
//  edgeThreshold.cpp
//  video-mush
//
//  Created by Josh McNamee on 26/10/2016.
//
//

#include <Mush Core/opencl.hpp>

#include "edgeThreshold.hpp"


edgeThreshold::edgeThreshold(float threshold) : mush::imageProcess(), _threshold(threshold) {
    
}

edgeThreshold::~edgeThreshold() {
    
}

void edgeThreshold::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 2);
    
    threshold = context->getKernel("edge_threshold");
    clear = context->getKernel("edge_clear");
    
    imageBuffe = castToImage(buffers.begin()[0]);
    imageBuffe->getParams(_width, _height, _size);
    
    laplaceBuffer = castToImage(buffers.begin()[1]);
    
    addItem((unsigned char *)context->redGreenImage(_width, _height));
    
    clear->setArg(0, _getImageMem(0));
    threshold->setArg(2, _getImageMem(0));
    threshold->setArg(3, _threshold);
    queue = context->getQueue();
}

void edgeThreshold::process() {
    inLock();
    
    cl::Event event;
    queue->enqueueNDRangeKernel(*clear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    auto image = imageBuffe->outLock();
    if (image == nullptr) {
        release();
        return;
    }
    
    auto laplace = laplaceBuffer->outLock();
    if (laplace == nullptr) {
        release();
        return;
    }
    
    threshold->setArg(0, image.get_image());
    threshold->setArg(1, laplace.get_image());
    queue->enqueueNDRangeKernel(*threshold, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    
    laplaceBuffer->outUnlock();
    imageBuffe->outUnlock();
    inUnlock();
}
