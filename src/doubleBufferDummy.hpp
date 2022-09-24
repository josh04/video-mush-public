//
//  doubleBufferDummy.hpp
//  video-mush
//
//  Created by Josh McNamee on 11/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_doubleBufferDummy_hpp
#define media_encoder_doubleBufferDummy_hpp

#include "dll.hpp"
#include <Mush Core/imageBuffer.hpp>

namespace mush {
    class doubleBuffer;
	class VIDEOMUSH_EXPORTS doubleBufferDummy : public imageBuffer {
    public:
        doubleBufferDummy(doubleBuffer * buffer, bool isSecond) : mush::imageBuffer(), _buffer(buffer), _isSecond(isSecond) {
            
        }
        
        ~doubleBufferDummy() {
            
        }
        
        
        virtual void destroy();
        
        virtual mush::buffer& inLock();
        
        virtual void inUnlock();
        
        virtual const mush::buffer outLock(int64_t time);
        
        virtual void outUnlock();
        
        virtual bool good();

        virtual void getParams(unsigned int &width, unsigned int &height, unsigned int &size);
        
    private:
        doubleBuffer * _buffer;
        bool _isSecond = false;
        
    };

}
#endif
