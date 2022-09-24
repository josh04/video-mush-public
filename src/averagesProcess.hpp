//
//  averagesProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 11/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_averagesProcess_hpp
#define media_encoder_averagesProcess_hpp

#include <array>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class averagesProcess : public mush::guiAccessible, public mush::processNode {
public:
	averagesProcess() : mush::guiAccessible() {
        
    }
    
    ~averagesProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        
        addItem((unsigned char *)&ao);
        addItem((unsigned char *)&bo);
        
        average = context->getKernel("averages");
        
        
        
        buffer = castToImage(buffers.begin()[0]);
        
        buffer->getParams(_width, _height, _size);
        
        
        queue = context->getQueue();
        
        hostReadBuffer = (float *)context->hostReadBuffer(limit*4*sizeof(float));
        
		a = (cl::Image2D *)context->floatImage(_width, _height);
		b = (cl::Image2D *)context->floatImage(_width, _height);
//        b = (cl::Image2D *)context->floatImage(CL_MEM_READ_WRITE, ceil((double)_width / (double)scale), ceil((double)_height / (double)scale));
    }
    
    void process() {
        cl::Event event;
		cl::size_t<3> origin, region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _width; region[1] = _height; region[2] = 1;
        
        auto input = buffer->outLock();
        if (input == nullptr) {
            return;
        }
        queue->enqueueCopyImage(input.get_image(), *a, origin, origin, region, NULL, &event);
        event.wait();
        
        buffer->outUnlock();
        
        cl::Image2D *tmp;
        
        unsigned int x = _width; unsigned int y = _height;
        
        average->setArg(4, scale);
        
        // OpenCL
        while (x * y > limit) {
            average->setArg(0, *a);
            average->setArg(1, *b);
            average->setArg(2, x);
            average->setArg(3, y);
            
            x = ceil((double)x / (double)scale); if (x < 1) {x = 1;}
            y = ceil((double)y / (double)scale); if (y < 1) {y = 1;}
            queue->enqueueNDRangeKernel(*average, cl::NullRange, cl::NDRange(x, y), cl::NullRange, NULL, &event);
            event.wait();
            /*
             queue->enqueueReadImage(*a, CL_TRUE, origin, region, 0, 0, pixData, NULL, &event);
             event.wait();
             */
            tmp = a; a = b; b = tmp;
        }
        // CPU
        
        if (x * y > 1) {
            //		tmp = a; a = b; b = tmp;
            region[0] = x; region[1] = y;
            double color = 0.0;
            queue->enqueueReadImage(*a, CL_TRUE, origin, region, 0, 0, hostReadBuffer, NULL, &event);
            event.wait();
            
            float lmin = 1.0f, lmax = 0.0f, avg = 0.0f;
            
            for (unsigned int i = 0; i < x * y; ++i) {
                color += hostReadBuffer[i * 4];
                lmax = (lmax < hostReadBuffer[i * 4 + 1]) ? hostReadBuffer[i * 4 + 1] : lmax;
                lmin = (lmin > hostReadBuffer[i * 4 + 2]) ? hostReadBuffer[i * 4 + 2] : lmin;
                avg += hostReadBuffer[i * 4 + 3];
            }
            
            color = color / (double)(x * y);
            
            hostReadBuffer[0] = color;
            hostReadBuffer[1] = lmax;
            hostReadBuffer[2] = lmin;
            hostReadBuffer[3] = avg;
            
        } else {
            region[0] = 1; region[1] = 1;
            queue->enqueueReadImage(*a, CL_TRUE, origin, region, 0, 0, hostReadBuffer, NULL, &event);
            event.wait();
        }
        
        std::array<float, 4> * output = (std::array<float, 4> *)inLock().get_pointer();
        
        (*output)[0] = expf(hostReadBuffer[0]);												// hmean
        (*output)[1] = expf(hostReadBuffer[1]);								// max luminance
        (*output)[2] = expf(hostReadBuffer[2]);								// min luminance
        (*output)[3] = expf(hostReadBuffer[3] / (double)(_width * _height));			// mean
        inUnlock();
    }
    
private:
    cl::Kernel * average = nullptr;
    mush::registerContainer<mush::imageBuffer> buffer;
    
    std::array<float, 4> ao, bo;
    float * hostReadBuffer = nullptr;
    cl::Image2D * a = nullptr, * b = nullptr;
    const unsigned int scale = 4;
	const unsigned int limit = 10;
};

#endif

