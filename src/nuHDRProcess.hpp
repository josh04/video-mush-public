//
//  newHDrProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 22/01/2015.
//
//

#ifndef video_mush_newHDrProcess_hpp
#define video_mush_newHDrProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class nuHDRProcess : public mush::imageProcess {
public:
    nuHDRProcess() : mush::imageProcess() {
        
    }
    
    ~nuHDRProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 2);
        nuhdr = context->getKernel("chroma");
        
        originalImageBuffer = castToImage(buffers.begin()[0]);
        luminanceImageBuffer = castToImage(buffers.begin()[1]);
        if (originalImageBuffer.get() == nullptr || luminanceImageBuffer.get() == nullptr) {
            putLog("nuHDR kernel: wrong buffers");
            return;
        }
        
        originalImageBuffer->getParams(_width, _height, _size);
        
        addItem(context->floatImage(_width, _height));
        
        nuhdr->setArg(2, _getMem(0).get_buffer());
        queue = context->getQueue();
    }
    
    void process() {
        inLock();
        auto input = originalImageBuffer->outLock();
        auto lum = luminanceImageBuffer->outLock();
        if (input == nullptr) {
            release();
            return;
        }
        if (lum == nullptr) {
            release();
            return;
        }
        nuhdr->setArg(0, input.get_image());
        nuhdr->setArg(1, lum.get_image());
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*nuhdr, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        originalImageBuffer->outUnlock();
        luminanceImageBuffer->outUnlock();
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue;
    cl::Kernel * nuhdr;
    mush::registerContainer<mush::imageBuffer> originalImageBuffer;
    mush::registerContainer<mush::imageBuffer> luminanceImageBuffer;
//    float gamma = 0.4545f;
    
};

#endif
