//
//  sandProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 13/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_sandProcess_hpp
#define video_mush_sandProcess_hpp

#define NOMINMAX

#include <random>
#include <ctime>
#include <algorithm>

#include <Mush Core/opencl.hpp>
#include <Mush Core/imageProcess.hpp>

class sandProcess : public mush::imageProcess {
public:
    sandProcess(unsigned int width, unsigned int height) : mush::imageProcess(), gen(std::time(0)), w_rand(std::bind(std::uniform_int_distribution<int>(-10,10), gen)) {
        _width = width;
        _height = height;
    }
    
    ~sandProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        //assert(buffers.size() == 0);
        
        sand_add = context->getKernel("sand_add");
        sand_process = context->getKernel("sand_process");
        sand_to_image = context->getKernel("sand_to_image");
        sand_clear = context->getKernel("sand_clear");
        sand_bottom_bump = context->getKernel("sand_bottom_bump");
        sand_copy_stills = context->getKernel("sand_copy_stills");
        sand_add_bumpers = context->getKernel("sand_add_bumpers");
        
        sandLocations = context->intNotNormalisedImage(_width, _height);
        sandLocationsScratch = context->intNotNormalisedImage(_width, _height);
        
        sand_add_locations = context->buffer(_width * sizeof(cl_uchar));
        sand_add_buffer = (cl_uchar *)context->hostWriteBuffer(_width * sizeof(cl_uchar));
        
        addItem(context->floatImage(_width, _height));
        
        
        queue = context->getQueue();
        
        cl::Event event;
        sand_clear->setArg(0, *sandLocations);
        queue->enqueueNDRangeKernel(*sand_clear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        sand_add_bumpers->setArg(0, *sandLocations);
        queue->enqueueNDRangeKernel(*sand_add_bumpers, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
    }
    
    void process() {
        cl::Event event;
//        auto bool_rand = std::bind(std::uniform_int_distribution<int>(0,1), gen);
        
        sand_clear->setArg(0, *sandLocationsScratch);
        queue->enqueueNDRangeKernel(*sand_clear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        sand_bottom_bump->setArg(0, *sandLocations);
        sand_bottom_bump->setArg(1, *sandLocationsScratch);
        queue->enqueueNDRangeKernel(*sand_bottom_bump, cl::NDRange(0, _height-1), cl::NDRange(_width, 1), cl::NullRange, NULL, &event);
        event.wait();
        
        sand_process->setArg(0, *sandLocations);
        sand_process->setArg(1, *sandLocations);
        sand_process->setArg(2, *sandLocationsScratch);
        sand_process->setArg(3, 0);
        queue->enqueueNDRangeKernel(*sand_process, cl::NullRange, cl::NDRange(_width, _height-1), cl::NullRange, NULL, &event);
        event.wait();
        
        sand_process->setArg(0, *sandLocations);
        sand_process->setArg(1, *sandLocations);
        sand_process->setArg(2, *sandLocationsScratch);
        sand_process->setArg(3, -1);
        queue->enqueueNDRangeKernel(*sand_process, cl::NullRange, cl::NDRange(_width, _height-1), cl::NullRange, NULL, &event);
        event.wait();
        
        sand_process->setArg(0, *sandLocations);
        sand_process->setArg(1, *sandLocations);
        sand_process->setArg(2, *sandLocationsScratch);
        sand_process->setArg(3, 1);
        queue->enqueueNDRangeKernel(*sand_process, cl::NullRange, cl::NDRange(_width, _height-1), cl::NullRange, NULL, &event);
        event.wait();
        
        sand_copy_stills->setArg(0, *sandLocations);
        sand_copy_stills->setArg(1, *sandLocationsScratch);
        queue->enqueueNDRangeKernel(*sand_copy_stills, cl::NullRange, cl::NDRange(_width, _height-1), cl::NullRange, NULL, &event);
        event.wait();
        
        memset(sand_add_buffer, 0, sizeof(cl_uchar) * _width);
        
        for (int i = 1; i < _width; ++i) {
            if (i % (_width/5) == 0) {
                int off = std::max(std::min(i + w_rand(), (int)_width), 0);
                sand_add_buffer[off] = 1;
            }
        }
        
        queue->enqueueWriteBuffer(*sand_add_locations, CL_TRUE, 0, _width*sizeof(cl_uchar), sand_add_buffer, NULL, &event);
        event.wait();
        
        sand_add->setArg(0, *sand_add_locations);
        sand_add->setArg(1, *sandLocationsScratch);
        sand_add->setArg(2, *sandLocationsScratch);
        queue->enqueueNDRangeKernel(*sand_add, cl::NullRange, cl::NDRange(_width, 1), cl::NullRange, NULL, &event);
        event.wait();
        
        inLock();
        
        sand_to_image->setArg(0, *sandLocationsScratch);
        sand_to_image->setArg(1, _getImageMem(0));
        queue->enqueueNDRangeKernel(*sand_to_image, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        inUnlock();
        
        cl::size_t<3> origin, region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _width; region[1] = _height; region[2] = 1;
        
        queue->enqueueCopyImage(*sandLocationsScratch, *sandLocations, origin, origin, region, NULL, &event);
        event.wait();
         
    }
    
private:
    bool first_run = true;
    
    cl::Image2D * sandLocations = nullptr;
    cl::Image2D * sandLocationsScratch = nullptr;
    cl::Buffer * sand_add_locations = nullptr;
    
    cl_uchar * sand_add_buffer = nullptr;
    
    cl::CommandQueue * queue = nullptr;
    
    cl::Kernel * sand_add = nullptr;
    cl::Kernel * sand_process = nullptr;
    cl::Kernel * sand_to_image = nullptr;
    cl::Kernel * sand_clear = nullptr;
    cl::Kernel * sand_bottom_bump = nullptr;
    cl::Kernel * sand_copy_stills = nullptr;
    cl::Kernel * sand_add_bumpers = nullptr;
    
    std::mt19937 gen;
    std::function<int()> w_rand;
};

#endif
