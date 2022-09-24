#ifndef EXPORTINPUT_HPP
#define EXPORTINPUT_HPP

#include "ConfigStruct.hpp"
#include <Mush Core/frameGrabber.hpp>
#include <Mush Core/opencl.hpp>
#include <Mush Core/hdrExr.hpp>
#include "hdrProcessor.hpp"
#include "hdrTonemap.hpp"

class exportInput : public mush::frameGrabber {
public:
	exportInput() : mush::frameGrabber(mush::inputEngine::externalInput) {
	}
	
	~exportInput() {
	}
	
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
		_width = config.inputWidth;
		_height = config.inputHeight;
		
		switch (config.input_pix_fmt) {
			case mush::input_pix_fmt::char_4channel:
				_size = 1;
				break;
			case mush::input_pix_fmt::half_4channel:
				_size = 2;
				break;
			case mush::input_pix_fmt::float_4channel:
				_size = 4;
				break;
		}
		
		for (int i = 0; i < 2; i++) {
			addItem(context->hostWriteBuffer(_width*_height*4* _size));
		}
        

	}
	
	virtual void gather() {
		
	}
protected:
private:
};

#endif