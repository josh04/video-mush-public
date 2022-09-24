
//
//  registrationContainer.hpp
//  mush-core
//
//  Created by Josh McNamee on 22/11/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#ifndef registrationContainer_h
#define registrationContainer_h

#include <memory>

// pass it a ringbuffer and it'll addRepeat() it just like that!
namespace mush {
    template <class T>
    class registerContainer {
    public:
        registerContainer() : _ring_buffer(nullptr) {
            
        }
        
        registerContainer(const registerContainer& r) : _ring_buffer(r.get()) {
            if (_ring_buffer != nullptr) {
                _ring_buffer->addRepeat();
            }
        }
        
        registerContainer(std::shared_ptr<T> ring_buffer) : _ring_buffer(ring_buffer)  {
            if (_ring_buffer != nullptr) {
                _ring_buffer->addRepeat();
            }
        }

		registerContainer(std::nullptr_t) : _ring_buffer(nullptr) {
		}
        
        ~registerContainer() {
            if (_ring_buffer != nullptr) {
                _ring_buffer->removeRepeat();
            }
            _ring_buffer = nullptr;
        }
        
        registerContainer& operator=(std::shared_ptr<T> ptr) {
            if (_ring_buffer != nullptr) {
                _ring_buffer->removeRepeat();
            }
            _ring_buffer = ptr;
            if (ptr != nullptr) {
                _ring_buffer->addRepeat();
            }
            
            return *this;
        }
        registerContainer& operator=(T * ptr) {
            return *this = std::shared_ptr<T>(ptr);
        }

		bool operator==(std::nullptr_t) {
			return _ring_buffer == nullptr;
		}
        
        T& operator*() {
            return *_ring_buffer;
        }
        
        T* operator->() const {
            return _ring_buffer.get();
        }
        
        std::shared_ptr<T> get() const {
            return _ring_buffer;
        }
        
    private:
        std::shared_ptr<T> _ring_buffer;
    };
}


#endif /* registrationContainer_h */
