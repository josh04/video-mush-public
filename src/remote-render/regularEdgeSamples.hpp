//
//  regularEdgeSamples.hpp
//  video-mush
//
//  Created by Josh McNamee on 26/10/2016.
//
//

#ifndef regularEdgeSamples_h
#define regularEdgeSamples_h

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace cl {
    class Kernel;
    class CommandQueue;
}

class regularEdgeSamples : public mush::imageProcess {
public:
    regularEdgeSamples();
    ~regularEdgeSamples();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * samples = nullptr;
    cl::Kernel * clear = nullptr;
    
    unsigned int _width_full, _height_full;
    
    mush::registerContainer<mush::imageBuffer> imageBuffe;
};

#endif /* regularEdgeSamples_h */
