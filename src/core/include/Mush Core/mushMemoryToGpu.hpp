#ifndef MUSH_MEMORYTOGPU_HPP
#define MUSH_MEMORYTOGPU_HPP

#include "opencl.hpp"
#include "imageProcess.hpp"
#include "mush-core-dll.hpp"

/*
This class takes an image from main memory (hostMemory) and pushes it up to OpenCL.
The cl::Image2D * used can be accessed with outLock() and outUnlock()
*/
namespace mush {
	class memoryToGpu : public mush::imageProcess {
	public:
		memoryToGpu(unsigned int width, unsigned int height) : mush::imageProcess() {
			_width = width;
			_height = height;
		}

		~memoryToGpu() {}

		void init(std::shared_ptr<mush::opencl> context, std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
			if (buffers.size() != 0) {
				throw std::runtime_error("Buffers at memoryToGpu");
			}

			addItem(context->floatImage(_width, _height));

			_queue = context->getQueue();
		}

		void set_ptr(void * ptr) {
			_ptr = ptr;
		}

		void process() override {
			auto mem = inLock();
			if (mem == nullptr) {
				release();
				return;
			}

			cl::Event event;
			cl::size_t<3> origin, region;
			origin[0] = 0; origin[1] = 0; origin[2] = 0;
			region[0] = _width; region[1] = _height; region[2] = 1;

			_queue->enqueueWriteImage(mem.get_image(), CL_TRUE, origin, region, 0, 0, _ptr, NULL, &event);
			event.wait();


			inUnlock();
		}

	private:
		cl::CommandQueue * _queue = nullptr;
		void * _ptr = nullptr;
		//cl::Image2D * temp = nullptr;
		//cl::Kernel * rgba = nullptr;
	};
}
#endif
