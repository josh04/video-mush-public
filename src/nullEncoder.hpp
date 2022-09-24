//
//  nullEncoder.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_nullEncoder_hpp
#define media_encoder_nullEncoder_hpp


#include <Mush Core/registerContainer.hpp>

#include <Mush Core/encoderEngine.hpp>

class nullEncoder : public encoderEngine {
public:
	nullEncoder() : encoderEngine() {
		
	}
    
	~nullEncoder() {
	}
    
    virtual void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) {
        
        addItem(&done);
        
        _token = outBuffer->takeFrameToken();
        
        imageBuffer = outBuffer;
    }
    
    virtual std::shared_ptr<AVCodecContext> libavformatContext() { return nullptr; }
    
protected:
	void gather() {
        bool running = true;
        while (running) {
			auto buf = imageBuffer->outLock(0, _token);
            if (buf == nullptr && _token == -1) {
                running = false;
                break;
            }
            
            imageBuffer->outUnlock();
            
            inLock();
            inUnlock();
        }
        release();
	}
    
private:
    const bool done = true;
    
    mush::registerContainer<mush::ringBuffer> imageBuffer;
    
    size_t _token = -1;
};


#endif
