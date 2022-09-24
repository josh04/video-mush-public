//
//  bt709luminanceProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 14/01/2015.
//
//

#ifndef video_mush_bt709luminanceProcess_hpp
#define video_mush_bt709luminanceProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class bt709luminanceProcess : public mush::imageProcess {
    public:
        bt709luminanceProcess() : mush::imageProcess() {
            
        }
        
        ~bt709luminanceProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            bt709 = context->getKernel("bt709luminance");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            queue = context->getQueue();
        }
        
        void process() {
            inLock();
            auto input = buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            bt709->setArg(0, input.get_buffer());
            bt709->setArg(1, _getImageMem(0));
            
            cl::Event event;
            queue->enqueueNDRangeKernel(*bt709, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            buffer->outUnlock();
            inUnlock();
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * bt709 = nullptr;
        mush::registerContainer<mush::imageBuffer> buffer;
    };
    
}

#endif
