//
//  doubleBuffer.hpp
//  video-mush
//
//  Created by Josh McNamee on 11/06/2014.
//  Copyright (c) 2014. All rights reserved.
//


#include <Mush Core/ringBuffer.hpp>
#include "doubleBuffer.hpp"

using namespace mush;

void doubleBuffer::init(std::shared_ptr<mush::ringBuffer> buffer) {
    _buffer = buffer;
}

mush::buffer doubleBuffer::lockFirst() {
    std::unique_lock<std::mutex> uniqueLock(inL);
    if (_buffer == nullptr) {
		return mush::buffer{};
    }
    if (temp == nullptr) {
        temp = _buffer->outLock();
        _second = true;
        _first = true;
    }
    return temp;
}

void doubleBuffer::unlockFirst() {
    std::unique_lock<std::mutex> uniqueLock(outL);
    if (_buffer == nullptr) {
        return;
    }
    _first = false;
    
    if (_second == false) {
        _buffer->outUnlock();
		temp = mush::buffer{};
    }
}

mush::buffer doubleBuffer::lockSecond() {
    std::unique_lock<std::mutex> uniqueLock(inL);
    if (_buffer == nullptr) {
		return mush::buffer{};
    }
    if (temp == nullptr) {
        temp = _buffer->outLock();
        _second = true;
        _first = true;
    }
    return temp;
}

void doubleBuffer::unlockSecond() {
    std::unique_lock<std::mutex> uniqueLock(outL);
    if (_buffer == nullptr) {
        return;
    }
    _second = false;
    if (_first == false) {
        _buffer->outUnlock();
		temp = mush::buffer{};
    }
}

bool doubleBuffer::good() {
    return _buffer->good();
}

std::shared_ptr<mush::ringBuffer> doubleBuffer::getFirst() {
    return std::shared_ptr<mush::ringBuffer>(&firstDummy, [](mush::ringBuffer *){});
}

std::shared_ptr<mush::ringBuffer> doubleBuffer::getSecond() {
    return std::shared_ptr<mush::ringBuffer>(&secondDummy, [](mush::ringBuffer *){});
}

