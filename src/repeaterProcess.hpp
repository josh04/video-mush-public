//
//  laplaceProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 04/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_repeater_hpp
#define video_mush_repeater_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
namespace mush {
	class repeaterProcess : public mush::imageProcess {
	public:
		repeaterProcess() : mush::imageProcess() {

		}

		~repeaterProcess() {

		}

		void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
			assert(buffers.size() == 1);
			_copy = context->getKernel("copyImage");

			_buffer = castToImage(buffers.begin()[0]);
            _buffer_store = _buffer.get();

			_buffer->getParams(_width, _height, _size);

			addItem(context->floatImage(_width, _height));

			queue = context->getQueue();
		}
        
        void set_take_next() {
            _buffer = _buffer_store;
            empty[next] = true;
        }

		void process() {
			if (_buffer.get() != nullptr) {
				inLock();
				auto input = _buffer->outLock();
				if (input == nullptr) {
					release();
					return;
				}
				_copy->setArg(0, input.get_image());
				_copy->setArg(1, _getImageMem(0));

				cl::Event event;
				queue->enqueueNDRangeKernel(*_copy, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
				event.wait();

				_buffer->outUnlock();
				inUnlock();
			}
			_buffer = nullptr;
		}

		void inUnlock() {
			int nx = next;
			mush::imageBuffer::inUnlock();
			empty[nx] = false;
		}

		void outUnlock() {
			int nw = now;
			mush::imageBuffer::outUnlock();
			empty[nw] = false;
		}

	private:
		cl::Kernel * _copy = nullptr;
		mush::registerContainer<mush::imageBuffer> _buffer;
        std::shared_ptr<mush::imageBuffer> _buffer_store = nullptr;
	};
}
#endif
