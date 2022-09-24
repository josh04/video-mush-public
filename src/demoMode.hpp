//
//  demoMode.h
//  video-mush
//
//  Created by Josh McNamee on 08/08/2015.
//
//

#ifndef __video_mush__demoMode__
#define __video_mush__demoMode__

#include <Mush Core/imageProcessor.hpp>
#include "ConfigStruct.hpp"

namespace mush {
    class imageProcess;
    class frameStepper;
    class imageBuffer;
    class guiAccessible;
    class demoProcess;
}

namespace mush {
    class demoMode : public imageProcessor {
    public:
        demoMode();
        
        ~demoMode() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
        
        std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;
        
        void process() override;
        
        void go() override;
        
        void release();
        
        std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;
        
    private:
        bool running = true;
        
        std::shared_ptr<mush::imageBuffer> _input_buffer = nullptr;
        std::shared_ptr<mush::demoProcess> _demo_process = nullptr;
        
        std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
        
    };
}

#endif /* defined(__video_mush__demoMode__) */
