//
//  shortFalseColour.hpp
//  video-mush
//
//  Created by Josh McNamee on 14/01/2015.
//
//

#ifndef video_mush_shortFalseColour_hpp
#define video_mush_shortFalseColour_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include "yuvDepthClampProcess.hpp"

namespace mush {
    class shortFalseColour : public mush::imageBuffer, public mush::processNode {
    public:
        shortFalseColour() : mush::imageBuffer(), mush::processNode() {
            tagInGuiUseExposure = false;
        }
        
        ~shortFalseColour() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<yuvDepthClampProcess> buffer) {
            _bitDepth = buffer->getBitDepth();
            if (_bitDepth > 8) {
                pq = context->getKernel("shortFalseColour");
            } else {
                pq = context->getKernel("charFalseColour");
            }
            this->buffer = buffer;
            
            buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            pq->setArg(1, _getImageMem(0));
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
        int _bitDepth = 8;
    };
    
}

#endif
