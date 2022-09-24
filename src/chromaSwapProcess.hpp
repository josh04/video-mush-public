//
//  chromaSwapProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 24/03/2015.
//
//

#ifndef video_mush_chromaSwapProcess_hpp
#define video_mush_chromaSwapProcess_hpp

#include <Mush Core/registerContainer.hpp>

namespace mush {
    class chromaSwapProcess : public mush::imageProcess {
    public:
        chromaSwapProcess() : mush::imageProcess() {
            
        }
        
        ~chromaSwapProcess() {}
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 2);
            this->context = context;
            
            swap = context->getKernel("chromaSwap");
            
            lumaBuffer = castToImage(buffers.begin()[0]);
            chromaBuffer = castToImage(buffers.begin()[1]);
            
            lumaBuffer->getParams(_width, _height, _size);
            
            unsigned int widthC, heightC;
            chromaBuffer->getParams(widthC, heightC, _size);
            if (_width != widthC || _height != heightC) {
                putLog("Warning! Chroma swap width and height not equal.");
            }
            
            addItem(context->floatImage(_width, _height));
            queue = context->getQueue();
            
            swap->setArg(2, _getImageMem(0));
        }
        
        void process() {
            inLock();
            auto ptrL = lumaBuffer->outLock();
            
            if (ptrL == nullptr) {
                return;
            }
            auto ptrC = chromaBuffer->outLock();
            
            if (ptrC == nullptr) {
                return;
            }
            
            swap->setArg(0, ptrL.get_image());
            swap->setArg(1, ptrC.get_image());
            
            cl::Event event;
            queue->enqueueNDRangeKernel(*swap, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            lumaBuffer->outUnlock();
            chromaBuffer->outUnlock();
            inUnlock();
        }
        
    private:
        std::shared_ptr<mush::opencl> context = nullptr;
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * swap = nullptr;
        
        mush::registerContainer<mush::imageBuffer> lumaBuffer, chromaBuffer;
    };
}


#endif
