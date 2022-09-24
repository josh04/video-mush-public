//
//  singleKernelProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 05/08/2015.
//
//

#ifndef video_mush_singleKernelProcess_hpp
#define video_mush_singleKernelProcess_hpp

#include "mush-core-dll.hpp"

#include "opencl.hpp"
#include "imageProcess.hpp"
#include "registerContainer.hpp"

namespace mush {
   class  MUSHEXPORTS_API singleKernelProcess : public imageProcess {
    public:
		singleKernelProcess(const char * kernel_name) : mush::imageProcess(), _kernel_name(kernel_name) {}
        ~singleKernelProcess() {}
        
		virtual void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
		virtual void process() override;
		virtual void setArgs();
        
    protected:
        const char * _kernel_name;
        cl::CommandQueue * _queue = nullptr;
        cl::Kernel * _kernel = nullptr;
        mush::registerContainer<mush::imageBuffer> _buffer;
		mush::registerContainer<mush::imageBuffer> _buffer2;
    };
}

#endif
