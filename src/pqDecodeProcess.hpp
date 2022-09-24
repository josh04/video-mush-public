//
//  pqDecodeProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 13/01/2015.
//
//

#ifndef video_mush_pqDecodeProcess_hpp
#define video_mush_pqDecodeProcess_hpp


#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include "yuvDepthClampProcess.hpp"

namespace mush {
    class pqDecodeProcess : public mush::imageBuffer, public mush::processNode {
    public:
        pqDecodeProcess(float yuvMax, bool pqLegacy) : mush::imageBuffer(), mush::processNode(), _yuvMax(yuvMax), _yuvDepth(10), _pqLegacy(pqLegacy) {
            
        }
        
        ~pqDecodeProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<yuvDepthClampProcess> buffer) {
            
            _yuvDepth = buffer->getBitDepth();
            
            if (_yuvDepth > 8) {                
                if (_pqLegacy) {
                    pq = context->getKernel("pqdecodeShortLegacy");
                } else {
                    pq = context->getKernel("pqdecodeShort");
                }
            } else {
                if (_pqLegacy) {
                    pq = context->getKernel("pqdecodeCharLegacy");
                } else {
                    pq = context->getKernel("pqdecodeChar");
                }
            }
            
            this->buffer = buffer;
            
            
            buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            pq->setArg(1, _getImageMem(0));
            pq->setArg(2, _yuvMax);
            pq->setArg(3, _yuvDepth);
            queue = context->getQueue();
        }
        
        void process() {
            inLock();
            auto input = buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            pq->setArg(0, input.get_buffer());
            
            cl::Event event;
            queue->enqueueNDRangeKernel(*pq, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            inUnlock();
            buffer->outUnlock();
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * pq = nullptr;
        mush::registerContainer<yuvDepthClampProcess> buffer;
        float _yuvMax = 10000.0f;
        int _yuvDepth = 10;
        bool _pqLegacy = false;
    };
    
}


#endif
