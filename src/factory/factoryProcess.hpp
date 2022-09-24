//
//  factoryProcess.hpp
//  factory
//
//  Created by Josh McNamee on 24/03/2015.
//
//

#ifndef factory_factoryProcess_hpp
#define factory_factoryProcess_hpp


/*
 This class takes an image from main memory (hostMemory) and pushes it up to OpenCL.
 The cl::Image2D * used can be accessed with outLock() and outUnlock()
 */

namespace factory {
class factoryProcess : public mush::imageProcess {
public:
    factoryProcess() : mush::imageProcess() {
        
    }
    
    ~factoryProcess() {}
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) {
        this->context = context;
        _width = 1920;
        _height = 1080;
        hostMemory = (float *)context->hostWriteBuffer(_width*_height*sizeof(float) * 4);
        addItem(context->floatImage(_width, _height));
        queue = context->getQueue();
    }
    
    void process() {
        inLock();
        
        cl::Event event;
        cl::size_t<3> origin, region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _width; region[1] = _height; region[2] = 1;
        ++count;
        memset(hostMemory, count, _width*_height*sizeof(float) * 4);
        queue->enqueueWriteImage(*(cl::Image2D *)_getMem(0), CL_TRUE, origin, region, 0, 0, hostMemory, NULL, &event);
        event.wait();
        
        inUnlock();
    }
    
private:
    std::shared_ptr<mush::opencl> context = nullptr;
    cl::CommandQueue * queue = nullptr;
    float * hostMemory = nullptr;
    
    std::shared_ptr<mush::imageBuffer> inputBuffer = nullptr;
    
    int count = 0;
    
};
}
#endif
