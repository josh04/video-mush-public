//
//  pqProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 08/01/2015.
//
//

#ifndef video_mush_pqProcess_hpp
#define video_mush_pqProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class pqEncodeProcess : public mush::imageProcess {
    public:
        pqEncodeProcess(float yuvMax, bool pqLegacy) : mush::imageProcess(), _yuvMax(yuvMax), _pqLegacy(pqLegacy) {
            
        }
        
        ~pqEncodeProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            if (_pqLegacy) {
                pq = context->getKernel("pqencodeLegacy");
            } else {
                pq = context->getKernel("pqencode");
            }
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            pq->setArg(1, _getImageMem(0));
            pq->setArg(2, _yuvMax);
            queue = context->getQueue();
        }
        
        void process() {
            inLock();
            auto input = buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            pq->setArg(0, input.get_image());
            
            cl::Event event;
            queue->enqueueNDRangeKernel(*pq, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            buffer->outUnlock();
            inUnlock();
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * pq = nullptr;
        mush::registerContainer<mush::imageBuffer> buffer;
        float _yuvMax = 10000.0f;
        bool _pqLegacy = false;
    };
    
}

#endif
