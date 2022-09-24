//
//  mush::ringBuffer.cpp
//  video-mush
//
//  Created by Josh McNamee on 19/08/2014.
//  Copyright (c) 2014. All rights reserved.
//

#include "ringBuffer.hpp"
//#include "doubleBuffer.hpp"

using namespace mush;

void ringBuffer::addItem(mush::buffer ptr) {
    ++_buffers;
    _addItem(ptr);
    locks.push_back(std::make_shared<std::mutex>());
    empty.push_back(true);
//    held_locks.push_back(std::unique_lock<std::mutex>());
    outConds.push_back(std::make_shared<std::condition_variable>());
	frame_ids.push_back(0); 
	_repeats_set.push_back(0);
}

void ringBuffer::addItem(cl::Image2D * image) {
	addItem(mush::buffer(*image));
}

void ringBuffer::addItem(cl::Buffer * buffer) {
	addItem(mush::buffer(*buffer));
}

void ringBuffer::addItem(void * ptr) {
	addItem(mush::buffer(ptr));
}

void ringBuffer::addItem(const void *ptr) {
	addItem(mush::buffer(ptr));
}

//#include <boost/thread.hpp>
//#include <boost/chrono.hpp>

void sleep(int ml) {
	std::this_thread::sleep_for(std::chrono::duration<int64_t, std::micro>(ml));
}

void ringBuffer::destroy() {
    std::unique_lock<std::mutex> uniqueLock(readyMutex);
    
    kill();
    sleep(10);
    
    if (!empty.empty()) {
        for (auto e : empty) {
            if (!e) {
                sleep(50);
            }
        }
    }
    
}


mush::buffer& ringBuffer::inLock() {
    if (killswitch) { // gotta check twice because software is dumb
        return end_response;
    }
    
    if (!ready) {
        std::unique_lock<std::mutex> uniqueLock(readyMutex);
        ready = true;
    }
    
    std::unique_lock<std::mutex> uniqueLock(*locks[next]);
    readyCondition.notify_all();
    
    try {
		if (!(empty[next] || killswitch)) {
			outConds[next]->wait(uniqueLock, [&]() {return (empty[next] || killswitch); });
		}
    } catch(std::system_error& e) {
        //putLog(e.what());
        throw std::runtime_error("Condition variable failed. Probably not an issue.");
    }
    
    if (killswitch) {
        kill();
        return end_response;
    }
    
    //held_locks[next].swap(uniqueLock);
    
#ifdef _BUF_DEBUG
    std::stringstream strm;
    strm << "In Locked No. #" << next;
    inL.push_back(strm.str());
#endif
    
    return _getMem(next);
}

mush::buffer& ringBuffer::inLock(const uint32_t& frame_id) {
	auto& ptr = inLock();
	frame_ids[next] = frame_id;
    return ptr;
}

void ringBuffer::inUnlock() {
    //            std::unique_lock<std::mutex> uniqueLock;
    //            held_locks[next].swap(uniqueLock);
    
	auto current = next; // for semantic help here

    empty[current] = false; // we have filled the current
    
#ifdef _BUF_DEBUG
    std::stringstream strm;
    strm << "In Unlocked No. #" << next;
    inU.push_back(strm.str());
#endif

	if (_repeats == 0) { // if that next repeat count is whack, squash it
		empty[current] = true;
		_repeats_set[current] = 0;
		//now = (now + 1) % _buffers;
		//_unlockCount = 0;
	} else {

		next = (current + 1) % _buffers; // next = current + 1
		_repeats_set[current] = _repeats; // tag in the next repeat count

		outConds[current]->notify_all(); // let everyone know the new sitch
	}

}

const mush::buffer ringBuffer::outLock() {
    return outLock(0);
}


const mush::buffer ringBuffer::outLock(int64_t time) {
    if (!ready) {
        std::unique_lock<std::mutex> uniqueLock(readyMutex);
        ready = true;
    }
    
    if (killswitch) { // gotta check twice because software is dumb
        return mush::buffer();
    }

    std::unique_lock<std::mutex> uniqueLock(*locks[now]);
    /*
	bool temp_repeat = false;
	if (_repeats == 0) {
		temp_repeat = true;
		addRepeat();
	}
	*/
    bool signal = false;

/*	if (!running) {
		if (empty[now]) {
			killswitch = true;
		}
	}

	if (!((!empty[now]) || killswitch)) {*/
	try {
		if (time == 0) {
			if (!((!empty[now]) || killswitch)) {
				outConds[now]->wait(uniqueLock, [&]() -> bool {
					if (!running) {
						if (empty[now]) {
							killswitch = true;
						}
					}
					return ((!empty[now]) || killswitch);
				});
			}
		} else {
			signal = outConds[now]->wait_for(uniqueLock, std::chrono::nanoseconds(time), [&]() -> bool { if (!running) { if (empty[now]) { killswitch = true; } } return ((!empty[now]) || killswitch); });
		}
		//                outCond.wait(uniqueLock, [&]() -> bool { if (!running) { if (empty[now]) { killswitch = true; } } return ((!empty[now]) || killswitch); });
	} catch (std::system_error& e) {
		//putLog(e.what());
		throw std::runtime_error("Condition variable failed. Probably not an issue.");
	}
//	}
    /*            if (!signal) {
     killswitch = true;
     }*/
    
    if (empty[now]) {
        return mush::buffer();
    }
    
    if (killswitch) {
        kill();
        return mush::buffer();
    }
    
    //held_locks[now].swap(uniqueLock);
    
#ifdef _BUF_DEBUG
    std::stringstream strm;
    strm << "Out Locked No. #" << now;
    outL.push_back(strm.str());
#endif
    /*
	if (temp_repeat) {
		removeRepeat();
	}
    */
    return _getMem(now);
}
/*
void const * ringBuffer::outLock(int64_t time, uint32_t& frame_id) {
	auto ptr = outLock(time);
	frame_id = frame_ids[now];
	return ptr;
}
*/

const mush::buffer ringBuffer::outLock(int64_t time, size_t& token) {
    {
        std::unique_lock<std::mutex> lock(token_mutex);
        if (taken_frame_tokens.size() > token) {
            if (taken_frame_tokens[token]) {
                if (time > 0) {
                    
                    auto signal = token_cond.wait_for(lock, std::chrono::nanoseconds(time), [&]() -> bool {
                        return ((!taken_frame_tokens[token]) || killswitch);
                    });
                    
                    if (taken_frame_tokens[token]) {
                        if (killswitch) {
                            token = -1;
                        }
                        return mush::buffer();
                    }
                } else {
                    token_cond.wait(lock, [&]() -> bool {
                        return ((!taken_frame_tokens[token]) || killswitch);
                    });
                    
                    if (taken_frame_tokens[token]) {
                        token = -1;
                        return mush::buffer();
                    }
                }
            }
        }
    }
    auto ptr = outLock(time);
    if (ptr != nullptr) {
        std::lock_guard<std::mutex> lock(token_mutex);
        taken_frame_tokens[token] = true;
    } else {
        if (time == 0 || killswitch) {
            token = -1;
        }
    }
    return ptr;
}

void ringBuffer::outUnlock() {
    /*
    if (useDoubleBuff) {
        auto ptr = doubleBuff->getFirst();
        ptr->outUnlock();
        return;
    }
    */
    std::unique_lock<std::mutex> uniqueLock(*locks[now]);
    _unlockCount++;
    
    if (_unlockCount >= _repeats_set[now]) {
        empty[now] = true;
        outConds[now]->notify_all();
        
        
        //std::unique_lock<std::mutex> uniqueLock;
        //held_locks[now].swap(uniqueLock);
    #ifdef _BUF_DEBUG
        std::stringstream strm;
        strm << "Out Unlocked No. #" << now;
        outU.push_back(strm.str());
    #endif
        now = (now + 1) % _buffers;
        
        _unlockCount = 0;
        if (taken_frame_tokens.size() > 0) {
            std::lock_guard<std::mutex> lock(token_mutex);
            for (int i = 0; i < taken_frame_tokens.size(); ++i) {
                taken_frame_tokens[i] = false;
            }
            token_cond.notify_all();
        }
    }
}

bool ringBuffer::good() {
    bool good = false;
    
    for (int i = 0; i < empty.size(); ++i) {
        good = good || !empty[i];
    }
    
    good = (good || running) && !killswitch; // if there's a full buffer or the gather is still running, we're good. if kill is set, we are bad.
    
    return good;
}

void ringBuffer::release() {
    running = false;
/*    for (int i = 0; i < held_locks.size(); ++i) {
        std::unique_lock<std::mutex> uniqueLock;
        held_locks[i].swap(uniqueLock);
        //if (!uniqueLock.owns_lock()) {
        //	uniqueLock.release();
        //}
    }*/
    for (int i = 0; i < outConds.size(); ++i) {
        outConds[i]->notify_all();
    }
}

void ringBuffer::kill() {
/*    for (int i = 0; i < held_locks.size(); ++i) {
        std::unique_lock<std::mutex> uniqueLock;
        held_locks[i].swap(uniqueLock);
        if (!uniqueLock) {
            uniqueLock.release();
        }
		if (held_locks[i]) {
			held_locks[i].unlock();
        }
    }*/
    killswitch = true;
    for (int i = 0; i < outConds.size(); ++i) {
        outConds[i]->notify_all();
    }
    for (int i = 0; i < taken_frame_tokens.size(); ++i) {
        taken_frame_tokens[i] = false;
    }
    token_cond.notify_all();
}

void ringBuffer::addRepeat() {
    _repeats++;
}

void ringBuffer::removeRepeat() {
    _repeats--;
}

void ringBuffer::resetRepeats() {
    _repeats = 0;
}

void ringBuffer::null() {
    resetRepeats();
    //_null = true;
}

void ringBuffer::denull() {
    //_null = false;
}

void ringBuffer::disable() {
	if (!_disabled_output) {
		_disabled_output = true;
		_disabled_repeats = _repeats;
	}
	resetRepeats();
}

void ringBuffer::enable() {
	if (_disabled_output) {
		_disabled_output = false;
		_repeats = _disabled_repeats;
	}
}
/*
std::shared_ptr<ringBuffer> ringBuffer::addThreadSafeRepeat() {
    if (doubleBuff == nullptr) {
        doubleBuff = std::make_shared<mush::doubleBuffer>();
        doubleBuff->init(std::shared_ptr<mush::ringBuffer>(this, [](mush::ringBuffer * ) {} ));
        useDoubleBuff = true;
    }
    return doubleBuff->getSecond();
	return nullptr;
}
*/
size_t ringBuffer::takeFrameToken() {
    taken_frame_tokens.push_back(false);
    return taken_frame_tokens.size() - 1;
}


