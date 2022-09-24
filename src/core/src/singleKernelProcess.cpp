//
//  singleKernelProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 05/08/2015.
//
//

#include "opencl.hpp"
#include "imageProcess.hpp"
#include "registerContainer.hpp"
#include "singleKernelProcess.hpp"

namespace mush {
		void singleKernelProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
			assert(buffers.size() == 1 || buffers.size() == 2);

			_kernel = context->getKernel(_kernel_name);

			_buffer = castToImage(buffers.begin()[0]);

			if (buffers.size() > 1) {
				_buffer2 = castToImage(buffers.begin()[1]);
			}

			_buffer->getParams(_width, _height, _size);

			addItem(context->floatImage(_width, _height));

			_queue = context->getQueue();
		}

		void singleKernelProcess::process() {
			auto buf = inLock();
			auto input = _buffer->outLock();
			if (input == nullptr) {
				release();
				return;
			}
			buf.copy_parameters(input);

			mush::buffer input2{};
			if (_buffer2.get() != nullptr) {
				input2 = _buffer2->outLock();
				if (input2 == nullptr) {
					release();
					return;
				}
			}

			cl::Event event;

			_kernel->setArg(0, input.get_image());

			if (_buffer2.get() != nullptr) {
				_kernel->setArg(1, input2.get_image());
				_kernel->setArg(2, _getImageMem(0));
			} else {
				_kernel->setArg(1, _getImageMem(0));
			}
			setArgs();

			_queue->enqueueNDRangeKernel(*_kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();

			_buffer->outUnlock();
			if (_buffer2.get() != nullptr) {
				_buffer2->outUnlock();
			}
			inUnlock();
		}

		void singleKernelProcess::setArgs() {

		}

}
