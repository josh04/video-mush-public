//
//  bayerSharpenProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 30/11/2015.
//
//

#ifndef bayerSharpenProcess_h
#define bayerSharpenProcess_h

#include <Mush Core/singleKernelProcess.hpp>

namespace mush {
    class bayerSharpenProcess : public mush::singleKernelProcess {
    public:
        bayerSharpenProcess() : mush::singleKernelProcess("bayer_sharpen") {
            
        }
        
        ~bayerSharpenProcess() {
            
        }
        
        virtual void process() override {
            inLock();
            auto input = _buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            
            cl::Event event;
            
            _kernel->setArg(0, input.get_image());
            
            _queue->enqueueNDRangeKernel(*_kernel, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
            event.wait();
            
            _buffer->outUnlock();
            inUnlock();
        }
    
    private:
        
    };
    
}


#endif /* bayerSharpenProcess_h */
