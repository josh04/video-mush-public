//
//  dualisoUpsize.hpp
//  video-mush
//
//  Created by Josh McNamee on 28/11/2015.
//
//

#ifndef dualisoUpsize_h
#define dualisoUpsize_h

#include "azure/Event.hpp"
#include "azure/Eventable.hpp"
#include "azure/Events.hpp"
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/opencl.hpp>

namespace mush {
    class dualiso_upsize : public mush::imageProcess {
    public:
        dualiso_upsize(int mod, float fac) : mush::imageProcess(), mod(mod), fac(fac) {
            
        }
        
        ~dualiso_upsize() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            upsize = context->getKernel("dualiso_linear_upsize");
            _blur = context->getKernel("copyImage");

            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            _temp = context->floatImage(_width, _height);
            
            _blur->setArg(0, *_temp);
            _blur->setArg(1, _getImageMem(0));
            
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
            
            upsize->setArg(0, input.get_image());
            
            
            // dark stretch
            upsize->setArg(1, *_temp);
            upsize->setArg(2, mod);
            
            queue->enqueueNDRangeKernel(*upsize, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
            event.wait();
            
            queue->enqueueNDRangeKernel(*_blur, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            buffer->outUnlock();
            inUnlock();
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * upsize = nullptr;
        cl::Kernel * _blur = nullptr;
        cl::Image2D * _temp = nullptr;
        
        mush::registerContainer<mush::imageBuffer> buffer;
        
        float fac = 4.0f;
        uint8_t mod = 0;
    };
    
}


#endif /* dualisoUpsize_h */
