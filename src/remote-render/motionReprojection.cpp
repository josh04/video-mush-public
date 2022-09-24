//
//  motionReprojection.cpp
//  video-mush
//
//  Created by Josh McNamee on 06/02/2017.
//
//

#include <Mush Core/opencl.hpp>

#include "motionReprojection.hpp"


motionReprojection::motionReprojection() : mush::imageProcess() {
    
}

motionReprojection::~motionReprojection() {
    
}

void motionReprojection::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 2 || buffers.size() == 4 || buffers.size() == 5);
    
    
    copy = context->getKernel("copyImage");
    clear = context->getKernel("clear_image");
	motion_preprocess = context->getKernel("motion_preprocess");
    _input = castToImage(buffers.begin()[0]);
    _input->getParams(_width, _height, _size);
    
    _motion = castToImage(buffers.begin()[1]);
    
    if (buffers.size() > 2) {
        _small = castToImage(buffers.begin()[2]);
        _depth = castToImage(buffers.begin()[3]);
        upscale = context->getKernel("scale_image");
        motion_reprojection = context->getKernel("motion_reprojection_demo_with_depth");
    } else {
        upscale = clear;
        motion_reprojection = context->getKernel("motion_reprojection_demo");
    }
    _previous = context->floatImage(_width, _height);
    clear->setArg(0, *_previous);
    
    queue = context->getQueue();
    
    cl::Event event;
    queue->enqueueNDRangeKernel(*clear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    addItem(context->floatImage(_width, _height));
    
    if (buffers.size() == 5) {
        _gl = castToImage(buffers.begin()[4]);
    }
}

void motionReprojection::process() {
    inLock();
    
    cl::Event event;
	mush::buffer image{};
    
    update_previous = false;
    //if (update_previous) {
        image = _input->outLock();
        if (image == nullptr) {
            release();
            return;
        }
    //}
    /*
    if (update_previous) {
        copy->setArg(0, *image);
        copy->setArg(1, *_previous);
        queue->enqueueNDRangeKernel(*copy, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        _input->outUnlock();
        update_previous = false;
        return;
    }
    */
    auto motion = _motion->outLock();
    if (motion == nullptr) {
        release();
        return;
    }

	motion_preprocess->setArg(0, motion.get_image());
	motion_preprocess->setArg(1, motion.get_image());
	queue->enqueueNDRangeKernel(*motion_preprocess, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
	event.wait();

    if (_small.get() != nullptr && !update_previous) {
        auto sm = _small->outLock();
        if (sm == nullptr) {
            release();
            return;
        }
        
        upscale->setArg(0, sm.get_image());
        upscale->setArg(1, _getImageMem(0));
    } else {
        upscale->setArg(0, _getImageMem(0));
    }
    if (_depth.get() != nullptr && !update_previous) {
        auto depth = _depth->outLock();
        if (depth == nullptr) {
            release();
            return;
        }
        
        motion_reprojection->setArg(4, depth.get_image());
    }
    
    queue->enqueueNDRangeKernel(*upscale, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    motion_reprojection->setArg(0, image.get_image());
    motion_reprojection->setArg(1, motion.get_image());
    motion_reprojection->setArg(2, _getImageMem(0));
    motion_reprojection->setArg(3, _getImageMem(0));
    queue->enqueueNDRangeKernel(*motion_reprojection, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    
    if (_small.get() != nullptr) {
        _small->outUnlock();
    }
    if (_depth.get() != nullptr) {
        _depth->outUnlock();
    }
    _motion->outUnlock();
    
    _input->outUnlock();
    inUnlock();
}
