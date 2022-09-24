//
//  trayraceUpsample.hpp
//  trayrace
//
//  Created by Josh McNamee on 07/09/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#ifndef trayrace_trayraceUpsample_hpp
#define trayrace_trayraceUpsample_hpp

#include <Mush Core/registerContainer.hpp>
#include <Mush Core/imageProcess.hpp>

namespace cl {
    class Kernel;
    class CommandQueue;
    class Image2D;
}
namespace mush {
    class imageBuffer;
    class integerMapBuffer;
}

class trayraceUpsample : public mush::imageProcess {
public:
    trayraceUpsample() : mush::imageProcess() {
        
    }
    
    ~trayraceUpsample() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
    
    void process();
    
    void setParams(int count, float geomWeight, float depthWeight, float kWeight);
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * spatialClear = nullptr;
    cl::Kernel * scaleImage = nullptr;
    cl::Kernel * spatialUpsample = nullptr;
    
    mush::registerContainer<mush::integerMapBuffer> maps;
    mush::registerContainer<mush::imageBuffer> geometry;
    mush::registerContainer<mush::imageBuffer> depth;
    mush::registerContainer<mush::imageBuffer> redraw;
    mush::registerContainer<mush::imageBuffer> lowres;
    
    cl::Image2D * lowRes = nullptr;
    
    int count = 15;
    int scale = 4;
    
};

#endif
