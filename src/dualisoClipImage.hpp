//
//  dualiso_clip_image.hpp
//  video-mush
//
//  Created by Josh McNamee on 28/11/2015.
//
//

#ifndef dualiso_clip_image_h
#define dualiso_clip_image_h

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class dualiso_clip_image : public mush::imageProcess {
    public:
        dualiso_clip_image() : mush::imageProcess() {
            
        }
        
        ~dualiso_clip_image() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            clip_image = context->getKernel("clip_top_bottom");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            _height = _height - 2;
            
            addItem(context->floatImage(_width, _height));
            
            clip_image->setArg(1, _getImageMem(0));
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
            
            clip_image->setArg(0, input.get_image());
            
            queue->enqueueNDRangeKernel(*clip_image, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            
            buffer->outUnlock();
            inUnlock();
            
            //fac = fac + 0.1f;
            //std::cout << fac << std::endl;
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * clip_image = nullptr;
        
        mush::registerContainer<mush::imageBuffer> buffer;
        
    };
    
}

#endif /* dualiso_clip_image_h */
