//
//  testCardInput.hpp
//  video-mush
//
//  Created by Visualisation on 04/04/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_testCardInput_hpp
#define media_encoder_testCardInput_hpp

#include <Mush Core/opencl.hpp>
#include <Mush Core/frameGrabber.hpp>
#include <Mush Core/hdrExr.hpp>


class testCardInput : public mush::frameGrabber {
public:
	testCardInput() : mush::frameGrabber(mush::inputEngine::testCardInput) {
		
	}
	
	~testCardInput() {
		
	}
	
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		_width = 2048;
		_height = 1088;
		_size = 2;
		this->testCardPath = std::string(config.resourceDir) + std::string(config.testCardPath);
		
		addItem(context->hostWriteBuffer(_width*_height*4*_size));
        
	}
    
    void getDetails(mush::core::inputConfigStruct &config) override {
        config.inputSize = 2;
        config.inputBuffers = 1;
    }
	
	void gather() {
        auto ptr = inLock();
        hdrEXR::ReadEXR(ptr, testCardPath.c_str());
		inUnlock();
	}
	
protected:
	
	virtual void inUnlock() {
        int nx = next;
        mush::imageBuffer::inUnlock();
		empty[nx] = false;
	}
	
	virtual void outUnlock() {
        int nw = now;
        mush::imageBuffer::outUnlock();
		empty[nw] = false;
	}
protected:
private:
	std::string testCardPath;
};

#endif
