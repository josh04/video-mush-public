//
//  frameGrabber.hpp
//  video-mush
//
//  Created by Visualisation on 24/04/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_hdrFrameGrabber_hpp
#define media_encoder_hdrFrameGrabber_hpp

#include "mush-core-dll.hpp"
#include "inputConfig.hpp"
#include "imageBuffer.hpp"
#include "imageProcess.hpp"

namespace mush {
	class mushPreprocessor;
class MUSHEXPORTS_API frameGrabber : public mush::imageProcess {
public:
	frameGrabber(const mush::inputEngine inputEngine) : mush::imageProcess(), _inputEngine(inputEngine) {
        setTagInGuiName("Input");
	}
	
	~frameGrabber() {
		
	}
    
	void setConfig(const mush::core::inputConfigStruct c) {
		config = c;
	}

    //virtual void init(shared_ptr<mush::opencl> context, const mush::core::inputConfigStruct config) {}
    
    virtual void getDetails(mush::core::inputConfigStruct &config) {}
	
	void process() override {
		throw std::runtime_error("No 'process()' on frameGrabber.");
	}

	virtual void gather() = 0;
    
	void startThread();
	
	const mush::inputEngine inputEngine() const;
	
protected:
	const mush::inputEngine _inputEngine = mush::inputEngine::noInput;
	unsigned int _numberOfFrames;

	mush::core::inputConfigStruct config;
};

}
#endif
