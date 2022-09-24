//
//  sbsPackProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 13/03/2017.
//
//

#ifndef sbsUnpackProcess_hpp
#define sbsUnpackProcess_hpp

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

class sbsUnpackProcess : public mush::imageProcess {
public:
    sbsUnpackProcess(bool is_right);
    ~sbsUnpackProcess();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
    private:
    cl::Kernel * _unpack = nullptr;
    
    mush::registerContainer<mush::imageBuffer> _left;
    
    bool _is_right;
    
};

#endif /* sbsPackProcess_hpp */
