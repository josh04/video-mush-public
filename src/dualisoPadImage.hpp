//
//  dualisoPadImage.hpp
//  video-mush
//
//  Created by Josh McNamee on 28/11/2015.
//
//

#ifndef dualisoPadImage_h
#define dualisoPadImage_h


#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/opencl.hpp>

namespace mush {
    class dualiso_pad_image : public mush::imageProcess {
    public:
        dualiso_pad_image() : mush::imageProcess() {
            
        }
        
        ~dualiso_pad_image() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            pad_image = context->getKernel("pad_top_bottom");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            _height = _height + 2;
            
            addItem(context->floatImage(_width, _height));
            
            pad_image->setArg(1, _getImageMem(0));
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
            
            pad_image->setArg(0, input.get_image());
            
            queue->enqueueNDRangeKernel(*pad_image, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            
            buffer->outUnlock();
            inUnlock();
            
            //fac = fac + 0.1f;
            //std::cout << fac << std::endl;
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * pad_image = nullptr;
        
        mush::registerContainer<mush::imageBuffer> buffer;
        
    };
    
}

#endif /* dualisoPadImage_h */
