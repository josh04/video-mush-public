//
//  dualISOProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 27/11/2015.
//
//

#ifndef dualISOProcess_h
#define dualISOProcess_h


#include "azure/Event.hpp"
#include "azure/Eventable.hpp"
#include "azure/Events.hpp"
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/opencl.hpp>

namespace mush {
    class dualISOProcess : public mush::imageProcess {
    public:
        dualISOProcess() : mush::imageProcess() {
            
        }
        
        ~dualISOProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            upsize = context->getKernel("dualiso_upsize");
            scale = context->getKernel("dualiso_scale");
            merge = context->getKernel("dualiso_merge");
            transfer_function = context->getKernel("copyImage");
            
            pad_image = context->getKernel("pad_top_bottom");
            clip_image = context->getKernel("clip_top_bottom");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            _height = _height + 2;
            
            addItem(context->floatImage(_width, _height - 2));
            
            upsize_dark = context->floatImage(_width, _height);
            upsize_light = context->floatImage(_width, _height);
            scale_dark = context->floatImage(_width, _height);
            scale_light = context->floatImage(_width, _height);
            
            _input = context->floatImage(_width, _height);
            
            merge->setArg(0, *_input);
            merge->setArg(1, *upsize_dark);
            merge->setArg(2, *upsize_light);
            merge->setArg(3, *scale_dark);
            merge->setArg(4, *scale_light);
            merge->setArg(5, fac);
            
            transfer_function->setArg(0, _getImageMem(0));
            transfer_function->setArg(1, _getImageMem(0));
            
            pad_image->setArg(1, *_input);
            
            clip_image->setArg(0, *_input);
            clip_image->setArg(1, _getImageMem(0));
            
            queue = context->getQueue();
        }
        
        void process() {
            inLock();
            auto input = buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            merge->setArg(5, fac);
            
            cl::Event event;
            /*
             displace->setArg(0, input.get_image());
             
             queue->enqueueNDRangeKernel(*displace, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
             event.wait();
             */
            
            pad_image->setArg(0, input.get_image());
            
            queue->enqueueNDRangeKernel(*pad_image, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            upsize->setArg(0, *_input);
            scale->setArg(0, *_input);
            
            
            // dark stretch
            upsize->setArg(1, *upsize_dark);
            upsize->setArg(2, 1);
            
            queue->enqueueNDRangeKernel(*upsize, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
            event.wait();
            
            // light stretch
            upsize->setArg(1, *upsize_light);
            upsize->setArg(2, 0);
            
            queue->enqueueNDRangeKernel(*upsize, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
            event.wait();
            
            
            //dark scale
            
            scale->setArg(1, *scale_dark);
            scale->setArg(2, 0);
            scale->setArg(3, 1.0f/fac);
            
            queue->enqueueNDRangeKernel(*scale, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
            event.wait();
            
            
            // light scale
            
            scale->setArg(1, *scale_light);
            scale->setArg(2, 1);
            scale->setArg(3, fac);
            
            queue->enqueueNDRangeKernel(*scale, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
            event.wait();
            
            // merge
            
            queue->enqueueNDRangeKernel(*merge, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            queue->enqueueNDRangeKernel(*clip_image, cl::NullRange, cl::NDRange(_width, _height-2), cl::NullRange, NULL, &event);
            event.wait();
            
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height-2), cl::NullRange, NULL, &event);
            event.wait();
            
            
            buffer->outUnlock();
            inUnlock();
            
            //fac = fac + 0.1f;
            //std::cout << fac << std::endl;
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * upsize = nullptr;
        cl::Kernel * scale = nullptr;
        cl::Kernel * merge = nullptr;
        cl::Kernel * pad_image = nullptr;
        cl::Kernel * clip_image = nullptr;
        
        cl::Image2D * upsize_dark = nullptr, * upsize_light = nullptr, * scale_dark = nullptr, * scale_light = nullptr;
        
        cl::Image2D * _input = nullptr;
        
        cl::Kernel * transfer_function = nullptr;
        
        mush::registerContainer<mush::imageBuffer> buffer;
        
        float fac = 4.0f;
    };
    
}


#endif /* dualISOProcess_h */
