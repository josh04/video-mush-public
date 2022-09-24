//
//  displaceChannel.hpp
//  video-mush
//
//  Created by Josh McNamee on 03/05/2015.
//
//

#ifndef video_mush_displaceChannel_hpp
#define video_mush_displaceChannel_hpp


#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class displaceChannel : public mush::imageProcess {
    public:
        displaceChannel() : mush::imageProcess() {
            
        }
        
        ~displaceChannel() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            displace = context->getKernel("displaceChannel");
            downsample = context->getKernel("downsample");
            downsampleChromaRefit = context->getKernel("downsampleChromaRefit");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            
            displace->setArg(1, _getImageMem(0));
            
            downsampled = context->floatImage(_width/samples_w, _height/samples_h);
            
            downsample->setArg(1, *downsampled);
            downsample->setArg(2, samples_w);
            downsample->setArg(3, samples_h);
            
            
            
            downsampleChromaRefit->setArg(1, *downsampled);
            downsampleChromaRefit->setArg(2, _getImageMem(0));
            
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
            /*
            displace->setArg(0, *input);
            
            queue->enqueueNDRangeKernel(*displace, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
             */
            
            downsample->setArg(0, input.get_image());
            
            queue->enqueueNDRangeKernel(*downsample, cl::NullRange, cl::NDRange(_width/samples_w, _height/samples_h), cl::NullRange, NULL, &event);
            event.wait();
            
            downsampleChromaRefit->setArg(0, input.get_image());
            downsampleChromaRefit->setArg(3, time);
            ++time;
            
            uint32_t dist = rand() % 6;
            downsampleChromaRefit->setArg(4, dist);
            
            
            queue->enqueueNDRangeKernel(*downsampleChromaRefit, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            
            buffer->outUnlock();
            inUnlock();
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * displace = nullptr;
        
        cl::Kernel * downsample = nullptr;
        cl::Kernel * downsampleChromaRefit = nullptr;
        
        cl::Image2D * downsampled = nullptr;
        
        mush::registerContainer<mush::imageBuffer> buffer;
        
        const uint32_t samples_w = 32, samples_h = 4;
        
        uint32_t time = 0;
        
    };
    
}

#endif
