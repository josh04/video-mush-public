//
//  freeImageLoader.hpp
//  factory
//
//  Created by Josh McNamee on 25/03/2015.
//
//

#ifndef factory_freeImageLoader_hpp
#define factory_freeImageLoader_hpp

#include "FreeImagePlus.h"

/*
 This class takes an image from main memory (hostMemory) and pushes it up to OpenCL.
 The cl::Imag*e2D * used can be accessed with outLock() and outUnlock()
 */
class freeImageLoader : public mush::imageBuffer {
public:
    freeImageLoader() : mush::imageBuffer() {
        
    }
    
    ~freeImageLoader() {}
    
    void init(std::shared_ptr<mush::opencl> context, const char * path) {
        
        _image.load(path);
        
        _width = _image.getWidth();
        _height = _image.getHeight();
        
        auto i2 = _image.getImageType();
        auto i3 = _image.getColorType();
       
        
        addItem(context->floatImage(_width, _height));
        im = context->intImage(_width, _height);
        rgba = context->getKernel("rgba");
        rgba->setArg(0, *im);
        rgba->setArg(1, *(cl::Image2D *)_getMem(0));
        
        cl::CommandQueue * queue = context->getQueue();
        cl::Event event;
        
        uint8_t * boop = context->hostWriteBuffer(_width*_height*4*sizeof(uint8_t));
        memcpy(boop, _image.accessPixels(), _width*_height*3*sizeof(uint8_t));

        cl::size_t<3> origin, region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _width; region[1] = _height; region[2] = 1;
        
        queue->enqueueWriteImage(*im, CL_TRUE, origin, region, 0, 0, boop, NULL, &event);
        event.wait();
        
        queue->enqueueNDRangeKernel(*rgba, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        inLock();
        inUnlock();
    }
    
    virtual void inUnlock() {
        int nx = next;
        mush::imageBuffer::inUnlock();
        empty[nx] = false;
    }
    
    virtual void outUnlock() {
        int nw = now;
        mush::imageBuffer::outUnlock();
        empty[nw] = false;
    }
private:
    fipImage _image;
    cl::Image2D * im = nullptr;
    
    cl::Kernel * rgba = nullptr;
};

#endif
