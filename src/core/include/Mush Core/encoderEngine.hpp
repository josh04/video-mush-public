//
//  encoderEngine.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_encoderEngine_hpp
#define media_encoder_encoderEngine_hpp

struct AVCodecContext;

#include "outputConfig.hpp"

#include "guiAccessible.hpp"
#include "mush-core-dll.hpp"

class encoderEngine : public mush::ringBuffer {
public:
    encoderEngine() : mush::ringBuffer() {
        
    }
    
    ~encoderEngine() {
        
    }
    
	MUSHEXPORTS_API void startThread();
    
    virtual void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) = 0;
    
    virtual std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() {
        return std::vector<std::shared_ptr<mush::guiAccessible>>();
    }
    
    virtual std::shared_ptr<AVCodecContext> libavformatContext() = 0;
    
    //AVRational time_base;
protected:
    virtual void gather() = 0;
    unsigned int _outputWidth = 1280, _outputHeight = 720;
    unsigned int _inputWidth = 1280, _inputHeight = 720;
private:
};

#endif
