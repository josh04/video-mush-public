//
//  doubleBuffer.hpp
//  video-mush
//
//  Created by Josh McNamee on 11/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_doubleBuffer_hpp
#define media_encoder_doubleBuffer_hpp

#include <Mush Core/mushBuffer.hpp>
#include "dll.hpp"
#include "doubleBufferDummy.hpp"

namespace mush {
	class VIDEOMUSH_EXPORTS doubleBuffer {
    public:
        doubleBuffer() : firstDummy(this, false), secondDummy(this, true), _first(false), _second(false) {
            
        }
        
        ~doubleBuffer() {
            
        }
        
        void init(std::shared_ptr<mush::ringBuffer> buffer);
        
        mush::buffer lockFirst();
        
        void unlockFirst();
        
        mush::buffer lockSecond();
        
        void unlockSecond();
        
        bool good();
        
        std::shared_ptr<mush::ringBuffer> getFirst();
        
        std::shared_ptr<mush::ringBuffer> getSecond();
        
        virtual void getParams(unsigned int &width, unsigned int &height, unsigned int &size) {
            auto buf = std::dynamic_pointer_cast<mush::imageBuffer>(_buffer);
            if (buf != nullptr) {
                buf->getParams(width, height, size);
            }
        }
        
        void release() {
            _buffer = nullptr;
            firstDummy.release();
            secondDummy.release();
        }
        
    private:
        std::shared_ptr<mush::ringBuffer> _buffer = nullptr;
        mush::buffer temp;
        std::atomic<bool> _first, _second;
        doubleBufferDummy firstDummy, secondDummy;
        std::mutex inL, outL;
    };
    
}

#endif
