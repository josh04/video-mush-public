//
//  g8DecodeTest.hpp
//  video-mush
//
//  Created by Josh McNamee on 14/10/2015.
//
//

#ifndef g8DecodeTest_h
#define g8DecodeTest_h

#include <Mush Core/registerContainer.hpp>

namespace mush {
    class g8DecodeTest : public mush::imageProcess {
    public:
        g8DecodeTest(int yuvMax) : mush::imageProcess(), _yuvMax(yuvMax) {
            
        }
        
        ~g8DecodeTest() {}
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) {
            assert(buffers.size() == 1);
            
            this->context = context;
            
            gamma8 = context->getKernel("decodeGamma8");
            
            inputBuffer = castToImage(buffers.begin()[0]);
            inputBuffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            queue = context->getQueue();
            
            gamma8->setArg(1, _getImageMem(0));
            gamma8->setArg(2, _yuvMax);
        }
        
        void process() {
            inLock();
            auto ptr = inputBuffer->outLock();
            
            if (ptr == nullptr) {
                return;
            }
            
            gamma8->setArg(0, ptr.get_image());
            
            cl::Event event;
            queue->enqueueNDRangeKernel(*gamma8, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            inputBuffer->outUnlock();
            inUnlock();
        }
        
    private:
        std::shared_ptr<mush::opencl> context = nullptr;
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * gamma8 = nullptr;
        
        const float _yuvMax = 1000.0f;
        
        mush::registerContainer<mush::imageBuffer> inputBuffer;
    };
}

#endif /* g8DecodeTest_h */
