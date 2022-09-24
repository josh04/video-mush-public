
#include <Mush Core/ringBuffer.hpp>
#include <Mush Core/opencl.hpp>
#include "rotateImage.hpp"

namespace mush {

    void rotateImage::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        
        rotate = context->getKernel("rotateImage");
        
        buffer = castToImage(buffers.begin()[0]);
        
        buffer->getParams(_width, _height, _size);
        
        addItem(context->floatImage(_width, _height));
        
        rotate->setArg(1, _getImageMem(0));
        
        queue = context->getQueue();
    }

    void rotateImage::process() {
        inLock();
        auto input = buffer->outLock();
        if (input == nullptr) {
            release();
            return;
        }
        
        cl::Event event;
        
        rotate->setArg(0, input.get_image());
        rotate->setArg(2, (float)time);
        
        queue->enqueueNDRangeKernel(*rotate, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        ++time;
        buffer->outUnlock();
        inUnlock();
    }
    
}
