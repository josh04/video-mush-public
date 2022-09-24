//
//  barcodeProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 24/02/2015.
//
//

#ifndef video_mush_barcodeProcess_hpp
#define video_mush_barcodeProcess_hpp


#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class barcodeProcess : public mush::imageProcess {
    public:
        barcodeProcess() : mush::imageProcess() {
            
        }
        
        ~barcodeProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            barcode = context->getKernel("barcode");
            barcode_copy_slice = context->getKernel("barcode_copy_slice");
            barcode_copy_mix = context->getKernel("barcode_copy_mix");
            barcode_compress = context->getKernel("barcode_compress");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            tempImage = context->floatImage(_width, _height);
            addItem(context->floatImage(_width, _height));
            barcode_copy_slice->setArg(0, *tempImage);
            barcode_copy_slice->setArg(1, _getImageMem(0));
            barcode_copy_mix->setArg(0, *tempImage);
            barcode_copy_mix->setArg(1, _getImageMem(0));
            barcode_copy_mix->setArg(2, _getImageMem(0));
            
            barcode_compress->setArg(0, _getImageMem(0));
            barcode_compress->setArg(1, _getImageMem(0));
            
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
            
            unsigned int t = _width;
            
            barcode->setArg(0, input.get_image());
            barcode->setArg(1, *tempImage);
            
            while (t > 1) {
                t = t / 2;
                barcode->setArg(2, t);
            
                queue->enqueueNDRangeKernel(*barcode, cl::NullRange, cl::NDRange(t, _height), cl::NullRange, NULL, &event);
                event.wait();
                
                barcode->setArg(0, *tempImage);

            }
            if (_turn > 0) {
                if (_turn_tick > 0) {
                    barcode_copy_mix->setArg(3, _width-1);
                    barcode_copy_mix->setArg(4, ++_turn_tick);
                    queue->enqueueNDRangeKernel(*barcode_copy_mix, cl::NullRange, cl::NDRange(1, _height), cl::NullRange, NULL, &event);
                    event.wait();
                } else {
                    ++_turn_tick;
                    barcode_copy_slice->setArg(2, _width-1);
                    queue->enqueueNDRangeKernel(*barcode_copy_slice, cl::NullRange, cl::NDRange(1, _height), cl::NullRange, NULL, &event);
                    event.wait();
                    
                }
            } else {
                barcode_copy_slice->setArg(2, _count);
                queue->enqueueNDRangeKernel(*barcode_copy_slice, cl::NullRange, cl::NDRange(1, _height), cl::NullRange, NULL, &event);
                event.wait();
            }
            _count = (_count+1) % _width;
            
            if (_count == 0) {
                ++_turn;
            }
            
            if (_turn > 0 && _turn_tick == _turn) {
                _turn_tick = 0;
                if (!(_width - (_count + 2) == -1)) {
                    queue->enqueueNDRangeKernel(*barcode_compress, cl::NDRange(_count, 0), cl::NDRange(1, _height), cl::NullRange, NULL, &event);
                    event.wait();
                    
                    if (!(_width - (_count + 2) == 0)) {
                        barcode_copy_slice->setArg(0, _getImageMem(0));
                        barcode_copy_slice->setArg(1, *tempImage);
                        barcode_copy_slice->setArg(2, 0);
                        queue->enqueueNDRangeKernel(*barcode_copy_slice, cl::NDRange(_count+2, 0), cl::NDRange(_width - (_count+2), _height), cl::NullRange, NULL, &event);
                        event.wait();
                        
                        barcode_copy_slice->setArg(0, *tempImage);
                        barcode_copy_slice->setArg(1, _getImageMem(0));
                        barcode_copy_slice->setArg(2, -1);
                        queue->enqueueNDRangeKernel(*barcode_copy_slice, cl::NDRange(_count+2, 0), cl::NDRange(_width - (_count+2), _height), cl::NullRange, NULL, &event);
                        event.wait();
                    }
                }
                
            }
            
            buffer->outUnlock();
            inUnlock();
        }
        
        private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * barcode = nullptr;
        cl::Kernel * barcode_copy_slice = nullptr;
        cl::Kernel * barcode_copy_mix = nullptr;
        cl::Kernel * barcode_compress = nullptr;
        
        
        cl::Image2D * tempImage = nullptr;
        mush::registerContainer<mush::imageBuffer> buffer;
        float _yuvMax = 10000.0f;
        
        int _count = 0;
        unsigned int _turn = 0;
        unsigned int _turn_tick = 0;
        float weight = 1.0f;
        
    };
    
}


#endif
