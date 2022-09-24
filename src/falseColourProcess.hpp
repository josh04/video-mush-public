//
//  falseColourProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 13/01/2015.
//
//

#ifndef video_mush_falseColourProcess_hpp
#define video_mush_falseColourProcess_hpp

#include <Mush Core/gui.hpp>
#include <Mush Core/opencl.hpp>
#include "averagesProcess.hpp"
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/tagInGui.hpp>

namespace mush {
    class falseColourProcess : public mush::imageProcess {
    public:
        falseColourProcess() : mush::imageProcess() {
            tagInGuiUseExposure = false;
        }
        
        ~falseColourProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            fc = context->getKernel("falseColour");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            fc->setArg(1, _getImageMem(0));
            
            fc->setArg(2, 1.0f);
            
            queue = context->getQueue();
        }
        
        void process() {
            inLock();
            auto input = buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            fc->setArg(0, input.get_image());
            if (tagGui != nullptr) {
//                std::stringstream strm;
                float a = 0.0125 * pow(2.0, tagGui->gui->getExposure(0));
//                strm << a;
//                putLog(strm.str().c_str());
                fc->setArg(2, a);
            }
            
            cl::Event event;
            queue->enqueueNDRangeKernel(*fc, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            buffer->outUnlock();
            inUnlock();
        }
        
    private:
        float exposure = 1.0f;
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * fc = nullptr;
        mush::registerContainer<mush::imageBuffer> buffer;
    };
}

#endif
