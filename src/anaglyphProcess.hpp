//
//  anaglyphProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 19/02/2017.
//
//

#ifndef anaglyphProcess_hpp
#define anaglyphProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace cl {
    class Kernel;
    class CommandQueue;
}

class anaglyphProcess : public mush::imageProcess {
public:
    anaglyphProcess();
    ~anaglyphProcess();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
private:
    cl::Kernel * _anaglyph = nullptr;
    
    
    mush::registerContainer<mush::imageBuffer> _left;
    mush::registerContainer<mush::imageBuffer> _right;
    
    uint8_t _mode = 1;
};


#endif /* anaglyphProcess_hpp */
