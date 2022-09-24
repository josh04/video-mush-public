//
//  dualisoNeaten.hpp
//  video-mush
//
//  Created by Josh McNamee on 28/11/2015.
//
//

#ifndef dualisoNeaten_h
#define dualisoNeaten_h

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/opencl.hpp>

namespace mush {
    class dualiso_neaten : public mush::imageProcess {
    public:
        dualiso_neaten(float neaten, int mod) : mush::imageProcess(), neat(neaten), mod(mod) {
            
        }
        
        ~dualiso_neaten() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            neaten = context->getKernel("dualiso_neaten");
            
            _buffer = castToImage(buffers.begin()[0]);
            
            _buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            neaten->setArg(1, _getImageMem(0));
            neaten->setArg(2, mod);
            neaten->setArg(3, neat);
            
            queue = context->getQueue();
        }
        
        void process() {
            inLock();
            auto input = _buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            
            
            cl::Event event;
            
            // merge
            
            neaten->setArg(0, input.get_image());
            
            queue->enqueueNDRangeKernel(*neaten, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
            event.wait();
            
            _buffer->outUnlock();
            inUnlock();
            
        }
        
    private:
        mush::registerContainer<mush::imageBuffer> _buffer;
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * neaten = nullptr;
        
        uint8_t mod = 0;
        float neat = 1.1f;
    };
    
}

#endif /* dualisoNeaten_h */
