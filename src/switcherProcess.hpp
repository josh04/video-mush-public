//
//  switcherProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 20/01/2015.
//
//

#ifndef video_mush_switcherProcess_hpp
#define video_mush_switcherProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class switcherProcess : public mush::imageProcess {
    public:
        switcherProcess() : mush::imageProcess() {
            
        }
        
        ~switcherProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            for (auto buf : buffers) {
                auto buf2 = std::dynamic_pointer_cast<mush::imageBuffer>(buf);
                if (buf2 != nullptr) {
                    _buffers.push_back(mush::registerContainer<mush::imageBuffer>(buf2));
                }
            }
            
            _copyImage = context->getKernel("copyImage");
            
            if (buffers.size() < 1) {
                putLog("Switcher kernel: no buffers");
            }
            
            _numBuffers = _buffers.size();
            
            _width = 0;
            _height = 0;
            
            for (auto buffer : _buffers) {
                unsigned int width = 0, height = 0, size = 0;
                buffer->getParams(width, height, size);
                if (width > _width) {
                    _width = width;
                }
                if (height > _height) {
                    _height = height;
                }
                
                _widths.push_back(width);
                _heights.push_back(height);
            }
            
            addItem(context->floatImage(_width, _height));
            
            queue = context->getQueue();
            srand(time(NULL));
        }
        
        void switchChannel() {
            _next = (_current + (rand() % _numBuffers)) % _numBuffers;
        }
        
        void process() {
            tick++;
            if (tick > r) {
                r = rand() % 15;
                tick = 0;
                switchChannel();
            }
            if (_next != -1) {
                _current = _next;
                _next = -1;
            }
            
            inLock();
            auto input = _buffers[_current]->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            _copyImage->setArg(0, input.get_image());
            _copyImage->setArg(1, _getImageMem(0));
            
            cl::Event event;
            queue->enqueueNDRangeKernel(*_copyImage, cl::NullRange, cl::NDRange(_widths[_current], _heights[_current]), cl::NullRange, NULL, &event);
            event.wait();
            
            _buffers[_current]->outUnlock();
            
            inUnlock();
            
            for (int i = 0; i < _numBuffers; ++i) {
                if (i != _current) {
                    _buffers[i]->outLock();
                    _buffers[i]->outUnlock();
                }
            }
        }
        
    private:
        int r = 15;
        int tick = 0;
        int _current = 0;
        int _next = -1;
        size_t _numBuffers = 1;
        cl::Kernel * _copyImage = nullptr;
        std::vector<mush::registerContainer<mush::imageBuffer>> _buffers;
        
        std::vector<unsigned int> _widths, _heights;
    };
    
}

#endif
