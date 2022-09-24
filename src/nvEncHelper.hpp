#ifndef NVENCHELPER_HPP
#define NVENCHELPER_HPP

#include <Mush Core/ringBuffer.hpp>

class nvEncHelper : public mush::ringBuffer {
public:
	nvEncHelper() : mush::ringBuffer() {
		_buffers = 0;
	}

	~nvEncHelper() {

	}

	void add(unsigned char * ptr) {
		addItem(ptr);
		_buffers++;
	}

	std::vector<unsigned char*> get() {
		return mem;
	}

private:
};

#endif