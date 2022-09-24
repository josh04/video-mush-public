//
//  singleEXRInput.cpp
//  video-mush
//
//  Created by Josh McNamee on 20/09/2015.
//
//
#include "singleEXRInput.hpp"


#include <Mush Core/opencl.hpp>
#include <Mush Core/hdrEXR.hpp>

namespace mush {
    
    
    void singleEXRInput::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		if (path.size() == 0) {
			path = std::string(config.inputPath);
		}
        hdrEXR::ReadSize(path.c_str(), _width, _height);
        //_width = 2048;
        //_height = 1088;
        _size = 4;
        
        addItem(context->hostWriteBuffer(_width*_height*4*_size));
        
    }
    
    void singleEXRInput::getDetails(mush::core::inputConfigStruct &config) {
        config.inputSize = 2;
        config.inputBuffers = 1;
    }
    
    void singleEXRInput::gather() {
        auto ptr = inLock();
        hdrEXR::ReadEXR(ptr, path.c_str());
        inUnlock();
    }
    
    void singleEXRInput::inUnlock() {
        int nx = next;
        mush::imageBuffer::inUnlock();
        empty[nx] = false;
    }
    
    void singleEXRInput::outUnlock() {
        int nw = now;
        mush::imageBuffer::outUnlock();
        empty[nw] = false;
    }
}
