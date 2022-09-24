//
//  12bitprocess.hpp
//  video-mush
//
//  Created by Josh McNamee on 08/01/2015.
//
//

#ifndef video_mush__4202bitprocess_hpp
#define video_mush__4202bitprocess_hpp

#include "ConfigStruct.hpp"
#include <Mush Core/opencl.hpp>
#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/registerContainer.hpp>

#include <Mush Core/imageProcess.hpp>

namespace mush {
	class yuv422to420Process : public mush::integerMapProcess {
	public:

		yuv422to420Process(int bitDepth) : mush::integerMapProcess(bitDepth) {

		}

		~yuv422to420Process() {

		}

		using integerMapProcess::init;

		void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
			assert(buffers.size() == 1);

			buffer = castToIntegerMap(buffers.begin()[0]);
			buffer->getParams(_width, _height, _size);

			if (_bitDepth > 8) {
				//clamp = context->getKernel("clampFloatToYUV422Short");
				//imagesize = _width*_height * 2 * sizeof(uint16_t);
			} else {
				_422to420 = context->getKernel("chroma422to420Char");
				auto one_quarter_width = ceil(_width * 1.5);
				_size = one_quarter_width *_height * sizeof(uint8_t);
			}

			addItem(context->buffer(_size, CL_MEM_WRITE_ONLY));

			//gpuBuffer = context->buffer(imagesize, CL_MEM_WRITE_ONLY);


			queue = context->getQueue();
		}

		void process() {

			auto input = buffer->outLock();
			if (input == nullptr) {
				release();
				return;
			}

			auto output = inLock();
			if (output == nullptr) {
				release();
				return;
			}

			cl::Event event;
			_422to420->setArg(0, input.get_buffer());
			_422to420->setArg(1, output.get_buffer());
			_422to420->setArg(2, _bitDepth);
			_422to420->setArg(3, _width);
			_422to420->setArg(4, _height);
			queue->enqueueNDRangeKernel(*_422to420, cl::NullRange, cl::NDRange(_width / 2, _height / 2), cl::NullRange, NULL, &event);
			event.wait();

			inUnlock();
			buffer->outUnlock();
		}

		size_t get_buffer_int_width() const {
			if (_bitDepth > 8) {
				return sizeof(uint16_t);
			} else {
				return sizeof(uint8_t);
			}

		}

		size_t get_size() const {
			return _size;
		}

	private:
		std::shared_ptr<mush::integerMapBuffer> buffer;
		cl::Kernel * _422to420 = nullptr;

		//cl::CommandQueue * queue = nullptr;
	};
}
#endif

