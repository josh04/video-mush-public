//
//  trayraceCompose.hpp
//  trayrace
//
//  Created by Josh McNamee on 20/08/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#ifndef trayrace_trayraceCompose_hpp
#define trayrace_trayraceCompose_hpp

#include <array>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class trayraceCompose : public mush::imageProcess {
public:
    trayraceCompose() : mush::imageProcess() {
        
    }
    
    ~trayraceCompose() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 3);
        copyImage = context->getKernel("copyImage");
        //        fromCache = context->getKernel("fromCache");
        historyBuffer = context->getKernel("historyBuffer");
        spatiotemporalUpsample = context->getKernel("spatiotemporalUpsample");
        fromRay = context->getKernel("fromRay");
        spatialClear = context->getKernel("spatialClear");
        
        //rays = castToImage(buffers.begin()[0]);
        
        maps = castToIntegerMap(buffers.begin()[0]);
        
        motion = castToImage(buffers.begin()[1]);
        
        upsample = castToImage(buffers.begin()[2]);
        
        motion->getParams(_width, _height, _size);
        
        historyFrame = context->floatImage(_width, _height);
        temp = context->floatImage(_width, _height);
        
        queue = context->getQueue();
        
        cl::Event event;
        
        addItem(context->floatImage(_width, _height));
        
        fromRay->setArg(0, *temp);
        
        spatiotemporalUpsample->setArg(0, *temp);
        spatiotemporalUpsample->setArg(1, *historyFrame);
        
        historyBuffer->setArg(0, *historyFrame);
        historyBuffer->setArg(1, _getImageMem(0));
        historyBuffer->setArg(2, *temp);
        
        //cl::Event event;
        spatialClear->setArg(0, *historyFrame);
        queue->enqueueNDRangeKernel(*spatialClear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
//        fromCache->setArg(0, _getImageMem(0)));
//        fromCache->setArg(1, *previousFrame);
        
//        copyImage->setArg(0, _getImageMem(0)));
        copyImage->setArg(1, *historyFrame);
        
        
    }
    
    void process() {
        cl::Event event;
        
        inLock();
        
        
        auto map = maps->outLock();
        if (map == nullptr) {
            release();
            return;
        }
        
        //auto ray = rays->outLock();
        //if (ray == nullptr) {
        //    release();
        //    return;
        //}
        
        auto m = motion->outLock();
        if (m == nullptr) {
            release();
            return;
        }
        
        auto up = upsample->outLock();
        if (up == nullptr) {
            release();
            return;
        }
        
        //fromRay->setArg(1, *ray);
        //fromRay->setArg(2, *map);
        
        if (count == 0) {
            count++;
            copyImage->setArg(0, up.get_image());
            
            queue->enqueueNDRangeKernel(*copyImage, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
        }
        
        spatiotemporalUpsample->setArg(2, up.get_image());
        spatiotemporalUpsample->setArg(3, map.get_buffer());
        spatiotemporalUpsample->setArg(4, m.get_image());
        
        queue->enqueueNDRangeKernel(*spatiotemporalUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        upsample->outUnlock();
        motion->outUnlock();
        
        //queue->enqueueNDRangeKernel(*fromRay, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        //event.wait();
        
        //rays->outUnlock();
        maps->outUnlock();
        
        queue->enqueueNDRangeKernel(*historyBuffer, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * historyBuffer = nullptr;
    cl::Kernel * spatiotemporalUpsample = nullptr;
    cl::Kernel * fromRay = nullptr;
    cl::Kernel * spatialClear = nullptr;
    cl::Kernel * copyImage = nullptr;
    
    cl::Image2D * historyFrame = nullptr;
    cl::Image2D * temp = nullptr;
    
//    std::shared_ptr<mush::imageBuffer> rays;
    mush::registerContainer<mush::integerMapBuffer> maps;
    mush::registerContainer<mush::imageBuffer> motion;
    mush::registerContainer<mush::imageBuffer> upsample;
    
    int count = 0;
    
};

#endif
