//
//  chromaSmooth.hpp
//  video-mush
//
//  Created by Josh McNamee on 28/11/2015.
//
//

#ifndef chromaSmooth_h
#define chromaSmooth_h

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/opencl.hpp>

namespace mush {
    class chroma_smooth : public mush::imageProcess {
    public:
        chroma_smooth() : mush::imageProcess() {
            
        }
        
        ~chroma_smooth() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            _chroma_smooth = context->getKernel("chroma_smooth");
			_chroma_median = context->getKernel("chroma_median");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);

			_temp = context->floatImage(_width, _height);
            
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
            
			_chroma_median->setArg(0, input.get_image());
            
            
            // dark stretch
			_chroma_median->setArg(1, *_temp);
            
            queue->enqueueNDRangeKernel(*_chroma_median, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();

			_chroma_smooth->setArg(0, *_temp);
			_chroma_smooth->setArg(1, _getImageMem(0));

			queue->enqueueNDRangeKernel(*_chroma_smooth, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();
            
            buffer->outUnlock();
            inUnlock();
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * _chroma_smooth = nullptr;
		cl::Kernel * _chroma_median = nullptr;
		cl::Image2D * _temp = nullptr;
        
        mush::registerContainer<mush::imageBuffer> buffer;
            };
    
}

#endif /* chromaSmooth_h */
