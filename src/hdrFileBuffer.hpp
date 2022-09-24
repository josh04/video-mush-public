//
//  hdrFileBuffer.hpp
//  video-mush
//
//  Created by Visualisation on 08/03/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_hdrFileBuffer_hpp
#define media_encoder_hdrFileBuffer_hpp

#include <memory>
#include <string>
#include "ConfigStruct.hpp"
#include <Mush Core/scrubbableFrameGrabber.hpp>

class hdrFileBuffer : public mush::scrubbableFrameGrabber {
public:
	hdrFileBuffer() : mush::scrubbableFrameGrabber(mush::inputEngine::folderInput) {
        setTagInGuiName("Frame File Input");
    }
	~hdrFileBuffer() {}
	
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void getDetails(mush::core::inputConfigStruct &config) override;
	void gather() override;
    
    void moveToFrame(int frame) override;
    int getFrameCount() override;
    int getCurrentFrame() override;
	
protected:
	
private:
	std::string input;
	
	mush::filetype _filetype;
	int _pipe;
	
	bool _tiffFloat;
    bool _loopFrames = false;
    
    int _frameCount = 0;
    std::atomic_int _currentFrame{0};
    std::mutex _scrubMutex;
    bool _scrub = false;
    int _scrubTo = 0;
	
};

#endif
