#ifndef RGB16BITINPUT_HPP
#define RGB16BITINPUT_HPP

#include <iostream>
#include <fstream>
#include <memory>
#include <Mush Core/opencl.hpp>
#include <Mush Core/frameGrabber.hpp>

using std::ifstream;
using std::shared_ptr;
using std::make_shared;

class RGB16bitInput : public mush::frameGrabber {
public:
	RGB16bitInput()
		: mush::frameGrabber(mush::inputEngine::rgb16bitInput), input(nullptr) {
		_size = 2;
	}
    
	~RGB16bitInput() {

	}

	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		_width = config.inputWidth;
		_height = config.inputHeight;
		
		input = make_shared<ifstream>(config.inputPath, std::ios_base::in | std::ios_base::binary);

		if (_width * _height > 0) {
			for (int i = 0; i < config.inputBuffers*config.exposures; i++) {
				addItem(context->hostWriteBuffer(_width * _height * _size * 4));
			}
            
		}

	}

	void gather() {
		input->seekg(0, std::ios::beg);
		bool run = true;
		while (run) {
			auto buf = inLock();
			if (buf == nullptr) {
				return;
			}
			unsigned char * ptr = (unsigned char *)buf.get_pointer();
			input->read((char *)ptr, _width*_height*_size*3);
			int cnt = input->gcount();
			inUnlock();
			if (input->eof() || input->fail() || input->bad()) {
				run = false;
			}
		}
		release();
	}
protected:
private:
	shared_ptr<ifstream> input;
};

#endif