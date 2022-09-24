//
//  12bitprocess.hpp
//  video-mush
//
//  Created by Josh McNamee on 08/01/2015.
//
//

#ifndef video_mush__2bitprocess_hpp
#define video_mush__2bitprocess_hpp

#include "ConfigStruct.hpp"
#include <Mush Core/opencl.hpp>
#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/registerContainer.hpp>

#include <Mush Core/imageProcess.hpp>

namespace mush {
    class yuvDepthClampProcess : public mush::integerMapProcess {
    public:
        
        yuvDepthClampProcess(int bitDepth, float yuvMax, transfer tr = transfer::linear) : mush::integerMapProcess(bitDepth), _yuvMax(yuvMax), _tr(tr) {
            
        }
        
        ~yuvDepthClampProcess() {
            
        }
        
        using integerMapProcess::init;
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            buffer = castToImage(buffers.begin()[0]);
            _token = buffer->takeFrameToken();
            
            buffer->getParams(_width, _height, _size);
            
            if (_bitDepth > 8) {
                clamp = context->getKernel("clampFloatToYUV422Short");
                imagesize = _width*_height*2*sizeof(uint16_t);
            } else {
                clamp = context->getKernel("clampFloatToYUV422Char");
                imagesize = _width*_height*2*sizeof(uint8_t);
            }
            
            switch (_tr) {
                case transfer::g8:
                    transfer_kernel = context->getKernel("encodePTF4");
                    transfer_kernel->setArg(2, _yuvMax);
                    break;
                case transfer::pq:
                    transfer_kernel = context->getKernel("pqencode");
                    transfer_kernel->setArg(2, _yuvMax);
                break;
                case transfer::srgb:
                    transfer_kernel = context->getKernel("encodeSRGB");
                    break;
                case transfer::gamma:
                    transfer_kernel = context->getKernel("gamma");
                    break;
                case transfer::rec709:
                    transfer_kernel = context->getKernel("encodeRec709");
                    break;
                default:
                case transfer::linear:
                    transfer_kernel = nullptr;
                    break;
            }
            
            
            if (transfer_kernel != nullptr) {
                temp_image2 = context->floatImage(_width, _height);
            }
            
            addItem(context->buffer(imagesize, CL_MEM_READ_WRITE));
            addItem(context->buffer(imagesize, CL_MEM_READ_WRITE));
            
            //gpuBuffer = context->buffer(imagesize, CL_MEM_WRITE_ONLY);
            
            
            queue = context->getQueue();
        }
        
        void process() {
            
            auto input = buffer->outLock(0, _token);
            if (input == nullptr && _token == -1) {
                release();
                return;
            }
            
            auto output = inLock();
            if (output == nullptr) {
                release();
                return;
            }
            
            cl::Event event;
            if (transfer_kernel != nullptr) {
                transfer_kernel->setArg(0, input.get_image());
                transfer_kernel->setArg(1, *temp_image2);
                queue->enqueueNDRangeKernel(*transfer_kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
                event.wait();
                
                clamp->setArg(0, *temp_image2);
            } else {
                clamp->setArg(0, input.get_image());
            }
            
            clamp->setArg(1, output.get_buffer());
            clamp->setArg(2, _bitDepth);
            clamp->setArg(3, 1.0f); // WAS yuvMAX
            queue->enqueueNDRangeKernel(*clamp, cl::NullRange, cl::NDRange(_width/2, _height), cl::NullRange, NULL, &event);
            event.wait();
                        
            inUnlock();
            buffer->outUnlock();
        }
        
        size_t get_buffer_int_width() const {
            if (_bitDepth > 8) {
                return sizeof(uint16_t);
            } else {
                return sizeof(uint8_t);
            }
            
        }
        
    protected:
    private:
        //std::vector<uint8_t *> mem;
        
        mush::registerContainer<mush::imageBuffer> buffer;
        cl::Kernel * transfer_kernel = nullptr;
        cl::Kernel * clamp = nullptr;
        cl::Buffer * gpuBuffer = nullptr;
        cl::Kernel * shortBufferToImage = nullptr;
        cl::Image2D * temp_image2 = nullptr;
        unsigned int imagesize = 0;
        
        cl::CommandQueue * queue  = nullptr;
        
        float _yuvMax = 1.0f;
        transfer _tr = transfer::linear;
    
        size_t _token = -1;
    };
}
#endif

