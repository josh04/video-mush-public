//
//  demoProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 08/08/2015.
//
//

#ifndef video_mush_demoProcess_hpp
#define video_mush_demoProcess_hpp

#include <azure/Eventable.hpp>
#include <azure/Event.hpp>
#include <azure/Eventkey.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>
#include <Mush Core/tagInGui.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class demoProcess : public mush::imageProcess, public azure::Eventable {
    public:
        demoProcess(unsigned int max, unsigned int cycle_length) : mush::imageProcess(), _cycle_length(cycle_length) {
            //if (max > 9) {
            //    max = 9;
            //}
            _max = max;
            _enabled = false;
            _throw_switch = false;
            _set_enabled_next_unlock = false;
        }
        
        ~demoProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            _demo = context->getKernel("demo_grid");
            _copy = context->getKernel("copyImage");
            
            _buffer = castToImage(buffers.begin()[0]);
            _buffer->getParams(_width, _height, _size);
            
            _grid = context->floatImage(_width, _height);
            _demo->setArg(1, *_grid);
            
            for(int i = 0; i < _max; ++i) {
                addItem(context->floatImage(_width, _height));
            }
            
            _queue = context->getQueue();
        }
        
        void process() {
            //frameStepper::process();
            
            cl::Event event;
            
            
            if (!_enabled || _throw_switch) {
                auto ptr = inLock();
                auto input = _buffer->outLock();
                if (input == nullptr) {
                    release();
                    return;
                }
                
                _copy->setArg(0, input.get_image());
                _copy->setArg(1, ptr.get_image());
                
                _queue->enqueueNDRangeKernel(*_copy, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
                event.wait();
                
                _buffer->outUnlock();
                
                inUnlock();
                
                if (_throw_switch) {
                    ringBuffer::outLock();
                    ringBuffer::outUnlock();
                    _throw_switch = false;
                    _switch_thrown_run_out = true;
                }
            }
            
            cl_float4 border_color = {1.0f, 1.0f, 1.0f, 0.0f};
            
            int compare = (now + _cycle % _max);
            for (int j = 0; j < 3; ++j) {
                for (int i = 0; i < 3; ++i) {
                    auto ptr2 = _getImageMem(((now + i + (j*3)) % _max));
                    if (ptr2 == nullptr) {
                        release();
                        return;
                    }
                    _demo->setArg(0, ptr2);
                    _demo->setArg(2, i);
                    _demo->setArg(3, j);
                    int tm = i + (j*3);
                    if (tm < _cycle_length) {
                        border_color = {0.0f, 0.0f, 0.2f, 1.0f};
                    }
                    
                    if (_enabled && tm == compare) {
                        border_color = {0.5f, 0.0f, 0.2f, 1.0f};
                    }
                    
                    _demo->setArg(4, border_color);
                    
                    _queue->enqueueNDRangeKernel(*_demo, cl::NullRange, cl::NDRange(_width/3, _height/3), cl::NullRange, NULL, &event);
                    event.wait();
                    
                }
            }
            
            if (getTagInGuiMember() && tagGui != nullptr) {
                tagGui->copyImageIntoGuiBuffer(getTagInGuiIndex(), (*_grid)());
            }
            
            
        }
        
        virtual void inUnlock() {
            ringBuffer::inUnlock();
        }
        
        
        virtual void outUnlock() {
            if (!_enabled) {
                ringBuffer::outUnlock();
            }
            if (_set_enabled_next_unlock) {
                _set_enabled_next_unlock = false;
                _enabled = !_enabled;
            }
        }
        
        virtual const mush::buffer outLock(int64_t time) {
            if (_enabled || _switch_thrown_run_out) {
                _cycle = (_cycle + 1) % _cycle_length;
                if (_cycle != 0 || !_switch_thrown_run_out) {
                    auto ptr = _getImageMem((now + _cycle) % _max);
                    return ptr;
                }
                _switch_thrown_run_out = false;
            }
            
            auto ptr = ringBuffer::outLock(time);
            return ptr;
        }
        
        void release() {
            imageBuffer::release();
        }
        
        bool event(std::shared_ptr<azure::Event> event) {
            if (event->isType("keyDown")) {
                switch (event->getAttribute<azure::Key>("key")) {
                    case azure::Key::Backslash:
                        _set_enabled_next_unlock = true;
                        break;
                    case azure::Key::Space:
                        _throw_switch = true;
                        break;
                }
            }
            return false;
        }
        
    private:
        cl::CommandQueue * _queue = nullptr;
        cl::Kernel * _demo = nullptr;
        cl::Kernel * _copy = nullptr;
        mush::registerContainer<mush::imageBuffer> _buffer;
        
        cl::Image2D * _grid = nullptr;
        
        unsigned int _cycle = 0;
        const unsigned int _cycle_length = 1;
        unsigned int _max = 9;
        
        std::atomic<bool> _set_enabled_next_unlock;
        std::atomic<bool> _enabled;
        std::atomic<bool> _throw_switch;
        std::atomic<bool> _switch_thrown_run_out;
    };
    
}

#endif
