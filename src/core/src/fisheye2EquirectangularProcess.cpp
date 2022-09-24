//
//  fisheye2EquirectangularProcess.cpp
//  mush-core
//
//  Created by Josh McNamee on 11/01/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#include <fisheye2EquirectangularProcess.hpp>
#include <azure/eventkey.hpp>
#include <sstream>

namespace mush {
    fisheye2EquirectangularProcess::fisheye2EquirectangularProcess(float fov, float x_centre, float y_centre) : singleKernelProcess("fisheye2equirectangular_upwards"), _fov(fov), _x_centre(x_centre), _y_centre(y_centre) {
        
    }
    
    fisheye2EquirectangularProcess::~fisheye2EquirectangularProcess() {
        
    }
    
    void fisheye2EquirectangularProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        
        _kernel = context->getKernel(_kernel_name);
        
        _buffer = castToImage(buffers.begin()[0]);
        
        unsigned int width, height;
        _buffer->getParams(width, height, _size);
        
        _height = height;
        _width = _height * 2;
        
        
        addItem(context->floatImage(_width, _height));
        
        _kernel->setArg(1, _getImageMem(0));
        
        _queue = context->getQueue();
    }
    
    void fisheye2EquirectangularProcess::setArgs() {
        _kernel->setArg(2, _fov);
        _kernel->setArg(3, _x_centre);
        _kernel->setArg(4, _y_centre);
    }
    
    bool fisheye2EquirectangularProcess::event(std::shared_ptr<azure::Event> event) {
        if (event->isType("keyDown")) {
            auto key = event->getAttribute<azure::Key>("key");
            if (key == azure::Key::Up) {
                _y_centre += 0.001f;
                std::stringstream strm;
                strm << "Y Offset set to " << _y_centre << ".";
                putLog(strm.str());
            } else if (key == azure::Key::Down) {
                _y_centre -= 0.001f;
                std::stringstream strm;
                strm << "Y Offset set to " << _y_centre << ".";
                putLog(strm.str());
            } else if (key == azure::Key::Left) {
                _x_centre -= 0.001f;
                std::stringstream strm;
                strm << "X Offset set to " << _x_centre << ".";
                putLog(strm.str());
            } else if (key == azure::Key::Right) {
                _x_centre += 0.001f;
                std::stringstream strm;
                strm << "X Offset set to " << _x_centre << ".";
                putLog(strm.str());
            } else if (key == azure::Key::Period) {
                _fov += 1.0f;
                std::stringstream strm;
                strm << "FOV set to " << _fov << ".";
                putLog(strm.str());
            } else if (key == azure::Key::Comma) {
                _fov -= 1.0f;
                std::stringstream strm;
                strm << "FOV set to " << _fov << ".";
                putLog(strm.str());
            }
        }
        return false;
    }
}
