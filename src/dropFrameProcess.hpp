//
//  dropFrameProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 04/11/2016.
//
//

#ifndef dropFrameProcess_hpp
#define dropFrameProcess_hpp

#include <Mush Core/opencl.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class dropFrameProcess : public imageProcess {
public:
    dropFrameProcess(uint32_t interval);
    ~dropFrameProcess();
    
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        void process() override;
        
        
    private:
        const uint32_t _interval = 0;
        uint32_t _tick = 0;
        
        cl::Kernel * _copy = nullptr;
        
        mush::registerContainer<mush::imageBuffer> _input;
        
    };
    
}

#endif /* dropFrameProcess_hpp */
    
