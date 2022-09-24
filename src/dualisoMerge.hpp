//
//  dualisoMerge.hpp
//  video-mush
//
//  Created by Josh McNamee on 28/11/2015.
//
//

#ifndef dualisoMerge_h
#define dualisoMerge_h

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/opencl.hpp>

namespace mush {
    class dualiso_merge : public mush::imageProcess {
    public:
        dualiso_merge(float f) : mush::imageProcess(), fac(f) {
            
        }
        
        ~dualiso_merge() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 3);
            
            merge = context->getKernel("dualiso_merge2");
            
            upsize_dark = castToImage(buffers.begin()[0]);
            upsize_light = castToImage(buffers.begin()[1]);
            scale_light = castToImage(buffers.begin()[2]);
            
            upsize_dark->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            queue = context->getQueue();
        }
        
        void set(float x) {
            std::lock_guard<std::mutex> lock(_set_mutex);
            fac = x;
        }
        
        void process() {
            inLock();
            auto up_d = upsize_dark->outLock();
            if (up_d == nullptr) {
                release();
                return;
            }
            auto up_l = upsize_light->outLock();
            if (up_l == nullptr) {
                release();
                return;
            }
            
            auto sc_l = scale_light->outLock();
            if (sc_l == nullptr) {
                release();
                return;
            }
            
            
            cl::Event event;
            
            // merge
            
            merge->setArg(1, up_d.get_image());
            merge->setArg(2, up_l.get_image());
            merge->setArg(3, sc_l.get_image());
            
            merge->setArg(0, _getImageMem(0));
            {
                std::lock_guard<std::mutex> lock(_set_mutex);
                merge->setArg(4, fac);
            }
            
            queue->enqueueNDRangeKernel(*merge, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
            event.wait();
            
            upsize_dark->outUnlock();
            upsize_light->outUnlock();
            scale_light->outUnlock();
            inUnlock();
            
        }
        
    private:
        std::mutex _set_mutex;
        
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * merge = nullptr;
        
        mush::registerContainer<mush::imageBuffer> upsize_dark;
        mush::registerContainer<mush::imageBuffer> upsize_light;
        mush::registerContainer<mush::imageBuffer> scale_light;
        
        float fac = 4.0f;
    };
    
}

#endif /* dualisoMerge_h */
