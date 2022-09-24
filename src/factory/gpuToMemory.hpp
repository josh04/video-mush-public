//
//  gpuToMainMemory.hpp
//  factory
//
//  Created by Josh McNamee on 24/03/2015.
//
//

#ifndef factory_gpuToMainMemory_hpp
#define factory_gpuToMainMemory_hpp

#include <Mush Core/opencl.hpp>

/*
 This class takes an image from OpenCL and brings it down to main memory.
 The main memory pointer can be accessed with outLock() and outUnlock()
 */
class gpuToMemory : public mush::imageProcess {
public:
    gpuToMemory() : mush::imageProcess() {
        
    }
    
    ~gpuToMemory() {}
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) {
        assert(buffers.size() == 2);
        
        this->context = context;
        
        inputBuffer = castToImage(buffers.begin()[0]);
        inputBuffer->getParams(_width, _height, _size);
        
        addItem(context->hostReadBuffer(_width*_height*sizeof(float)*4));
        queue = context->getQueue();
    }
    
    void process() {
        inLock();
        cl::Image2D * ptr = (cl::Image2D *)inputBuffer->outLock();
        
        if (ptr == nullptr) {
            return;
        }
        
        cl::Event event;
        cl::size_t<3> origin, region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _width; region[1] = _height; region[2] = 1;
        
        queue->enqueueReadImage(*ptr, CL_TRUE, origin, region, 0, 0, _getMem(0), NULL, &event);
        event.wait();
        
        inputBuffer->outUnlock();
        inUnlock();
    }
    
private:
    std::shared_ptr<mush::opencl> context = nullptr;
    cl::CommandQueue * queue = nullptr;
    
    std::shared_ptr<mush::imageBuffer> inputBuffer = nullptr;
};

#endif
