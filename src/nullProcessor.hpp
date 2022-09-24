//
//  nullProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_nullProcessor_hpp
#define media_encoder_nullProcessor_hpp

#include <Mush Core/imageProcessor.hpp>

class nullProcessor : public mush::imageProcessor {
public:
    nullProcessor(): mush::imageProcessor() {
    }
    
    ~nullProcessor() {
        
    }
    
    virtual void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        _imageBuffer = buffers.begin()[0];
        _guiBuffers.push_back(castToImage(_imageBuffer));
    }
    
    virtual const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const {
        return {_imageBuffer};
    }
    
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() {
        const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
        _guiBuffers.clear();
        return buffs;
    }
    
    void process() {
        _imageBuffer->outLock();
        _imageBuffer->outUnlock();
    }
    
    void go() {
        while(_imageBuffer->good()) {
            //process();
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
        
        _imageBuffer->release();
    }
private:
    std::shared_ptr<mush::ringBuffer> _imageBuffer = nullptr;
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
};

#endif
