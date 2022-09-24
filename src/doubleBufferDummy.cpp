//
//  doubleBufferDummy.hpp
//  video-mush
//
//  Created by Josh McNamee on 11/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#include <memory>

#include <Mush Core/ringBuffer.hpp>

#include "doubleBuffer.hpp"
#include "doubleBufferDummy.hpp"

using namespace mush;
    
void doubleBufferDummy::destroy() {
}

mush::buffer& doubleBufferDummy::inLock() {
	return mush::buffer{};
}

void doubleBufferDummy::inUnlock() {
    
}

const mush::buffer doubleBufferDummy::outLock(int64_t time) {
    if (_isSecond) {
        return _buffer->lockSecond();
    } else {
        return _buffer->lockFirst();
    }
}

void doubleBufferDummy::outUnlock() {
    if (_isSecond) {
        return _buffer->unlockSecond();
    } else {
        return _buffer->unlockFirst();
    }
}

bool doubleBufferDummy::good() {
    return _buffer->good();
}


void doubleBufferDummy::getParams(unsigned int &width, unsigned int &height, unsigned int &size) {
    _buffer->getParams(width, height, size);
}