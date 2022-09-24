//
//  imageDownScale.hpp
//  parcore
//
//  Created by Josh McNamee on 25/06/2015.
//  Copyright (c) 2015 Josh McNamee. All rights reserved.
//

#ifndef parcore_imageDownScale_hpp
#define parcore_imageDownScale_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class imageDownScale : public mush::imageProcess {
public:
    imageDownScale(int sc) : mush::imageProcess(), _sc(sc) {
        
    }
    
    ~imageDownScale() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 2);
        scaleImage = context->getKernel("scaleImage");
        
        input = castToImage(buffers.begin()[0]);
        map = castToIntegerMap(buffers.begin()[1]);
        
        input->getParams(_width, _height, _size);
        
        _width = _width/_sc;
        _height = _height/_sc;
        
        addItem(context->floatImage(_width, _height));
        
        scaleImage->setArg(0, _getImageMem(0));
        
        queue = context->getQueue();
    }
    
    void process() {
        cl::Event event;
        
        inLock();
		
		bool released = false;

        auto in = input->outLock();
        if (in == nullptr) {
			released = true;
        }
        
        auto re = map->outLock();
        if (re == nullptr) {
			released = true;
        }

		if (released) {
			map->outUnlock();
			release();
			return;
		}

        scaleImage->setArg(1, in.get_image());
        scaleImage->setArg(2, re.get_buffer());
        scaleImage->setArg(3, _sc);
        queue->enqueueNDRangeKernel(*scaleImage, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        // END
        
        map->outUnlock();
        input->outUnlock();
        
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * scaleImage = nullptr;
    
    mush::registerContainer<mush::imageBuffer> input;
    mush::registerContainer<mush::integerMapBuffer> map;
    
    int _sc = 4;
};

#endif
