
#include "scaleImageProcess.hpp"

namespace mush {

	scaleImageProcess::scaleImageProcess(unsigned int width, unsigned int height) : mush::singleKernelProcess("scale_image") {
		_width = width;
		_height = height;
	}

	scaleImageProcess::~scaleImageProcess() {

	}

	void scaleImageProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		assert(buffers.size() == 1);

		_kernel = context->getKernel(_kernel_name);

		_buffer = castToImage(buffers.begin()[0]);

		//_buffer->getParams(_width, _height, _size);

		addItem(context->floatImage(_width, _height));

		_kernel->setArg(1, _getImageMem(0));

		_queue = context->getQueue();
	}
}
