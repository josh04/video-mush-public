//
//  delayProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 22/08/2014.
//
//

#ifndef video_mush_delayProcess_hpp
#define video_mush_delayProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class delayProcess : public imageProcess {
    public:
        delayProcess() : imageProcess() {
            
        }
        
        ~delayProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            buffer = castToImage(buffers.begin()[0]);
            
            copyImage = context->getKernel("copyImage");
            
            buffer->getParams(_width, _height, _size);
            
            addItem(context->floatImage(_width, _height));
            addItem(context->floatImage(_width, _height));
            
            void * temp = malloc(_width*_height*sizeof(cl_float4)*4);
            cl::Event event;
            queue = context->getQueue();
            cl::size_t<3> origin, region;
            origin[0] = 0; origin[1] = 0; origin[2] = 0;
            region[0] = _width; region[1] = _height; region[2] = 1;
            queue->enqueueWriteImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, temp, NULL, &event);
            event.wait();
            queue->enqueueWriteImage(_getImageMem(1), CL_TRUE, origin, region, 0, 0, temp, NULL, &event);
            event.wait();
            free(temp);
            inLock();
            inUnlock();
            
        }
        
        void process() {
            auto input = buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            auto output = inLock();
            if (output == nullptr) {
                release();
                return;
            }
            copyImage->setArg(0, input.get_image());
            copyImage->setArg(1, output.get_image());
            
            cl::Event event;
            queue->enqueueNDRangeKernel(*copyImage, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            inUnlock();
            buffer->outUnlock();
        }
        
    private:
        mush::registerContainer<imageBuffer> buffer;
        cl::Kernel * copyImage = nullptr;
    };
}

#endif
