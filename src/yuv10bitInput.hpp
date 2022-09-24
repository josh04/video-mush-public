//
//  yuv10bitInput.hpp
//  media-encoder
//
//  Created by Josh McNamee on 05/08/2014.
//  Copyright (c) 2014 video-mush. All rights reserved.
//

#ifndef media_encoder_yuv10bitInput_hpp
#define media_encoder_yuv10bitInput_hpp

#include <iostream>
#include <fstream>
#include <memory>
#include <Mush Core/opencl.hpp>
#include <Mush Core/frameGrabber.hpp>
#include "rgb16bitInput.hpp"

using std::ifstream;
using std::shared_ptr;
using std::make_shared;

class YUV10bitInput : public mush::frameGrabber {
public:
	YUV10bitInput()
    : mush::frameGrabber(mush::inputEngine::yuv10bitInput), input(nullptr) {
        _size = 2;
	}
    
	~YUV10bitInput() {
        
	}
    
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		_width = config.inputWidth;
		_height = config.inputHeight;
		
        input = std::make_shared<std::ifstream>(config.inputPath, std::ios_base::in | std::ios_base::binary);
        
		if (_width * _height > 0) {
			for (int i = 0; i < config.inputBuffers*config.exposures; i++) {
				addItem(context->hostWriteBuffer(_width * _height * _size * 2));
			}
		}
        
	}
    
	void gather() {
        SetThreadName("yuvInput");
		input->seekg(0, std::ios::beg);
		bool run = true;
		while (run) {
			auto buf = inLock();
			if (buf == nullptr) {
				return;
			}
			unsigned char * ptr = (unsigned char *)buf.get_pointer();
			input->read((char *)ptr, _width*_height*_size * 2);
			int cnt = input->gcount();
			inUnlock();
			if (input->eof() || input->fail() || input->bad()) {
				run = false;
			}
		}
		release();
	}
    
private:
    std::shared_ptr<std::ifstream> input = nullptr;
};

#endif
