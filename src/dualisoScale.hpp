//
//  dualisoScale.hpp
//  video-mush
//
//  Created by Josh McNamee on 28/11/2015.
//
//

#ifndef dualisoScale_h
#define dualisoScale_h

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/opencl.hpp>
#include <cmath>

namespace mush {
    class dualiso_scale : public mush::imageProcess {
    public:
        dualiso_scale(int mod, float fac) : mush::imageProcess(), mod(mod), fac(pow(fac, 2)) {
            
        }
        
        ~dualiso_scale() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            scale = context->getKernel("dualiso_scale");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            queue = context->getQueue();
        }
        
        void set(float x) {
            std::lock_guard<std::mutex> lock(_set_mutex);
            fac = pow(x, 2);
        }
        
        void process() {
            inLock();
			auto input = buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            
            cl::Event event;
            
            scale->setArg(0, input.get_image());
            
            
            // dark stretch
            scale->setArg(1, _getImageMem(0));
            scale->setArg(2, mod);
            
            {
                std::lock_guard<std::mutex> lock(_set_mutex);
                scale->setArg(3, fac);
            }
            
            queue->enqueueNDRangeKernel(*scale, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
            event.wait();
            
            buffer->outUnlock();
            inUnlock();
        }
        
    private:
        std::mutex _set_mutex;
        
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * scale = nullptr;
        
        mush::registerContainer<mush::imageBuffer> buffer;
        
        float fac = 4.0f;
		uint8_t mod = 0;
    };
    
}

#endif /* dualisoScale_h */
