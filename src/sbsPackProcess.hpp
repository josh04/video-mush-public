//
//  sbsPackProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 13/03/2017.
//
//

#ifndef sbsPackProcess_hpp
#define sbsPackProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace cl {
    class Kernel;
    class CommandQueue;
}

namespace mush {
    namespace camera {
        class base;
        class parBase;
        class camera_event_handler;
    }
}

class sbsPackProcess : public mush::imageProcess {
public:
    sbsPackProcess();
    ~sbsPackProcess();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
    private:
    cl::Kernel * _pack = nullptr;
    
    mush::registerContainer<mush::imageBuffer> _left;
    mush::registerContainer<mush::imageBuffer> _right;
    
};

#endif /* sbsPackProcess_hpp */
