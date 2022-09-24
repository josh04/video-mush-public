//
//  quarterImageProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 05/08/2015.
//
//

#ifndef video_mush_quarterImageProcess_hpp
#define video_mush_quarterImageProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class quarterImageProcess : public mush::imageProcess {
    public:
        quarterImageProcess() : mush::imageProcess() {
            
        }
        
        ~quarterImageProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            debayer = context->getKernel("quarter_image");
            
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
            
            cl::Event event;
            
            debayer->setArg(0, input.get_image());
            debayer->setArg(1, _getImageMem(0));
        
            queue->enqueueNDRangeKernel(*debayer, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            
            buffer->outUnlock();
            inUnlock();
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * debayer = nullptr;
        
        
        mush::registerContainer<mush::imageBuffer> buffer;
        
    };
    
}

#endif
