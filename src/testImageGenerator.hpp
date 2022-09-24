#pragma once

#include <Mush Core/opencl.hpp>
#include <Mush Core/imageBuffer.hpp>

namespace mush {
	class testImageGenerator : public imageBuffer {
	public:
		testImageGenerator(std::shared_ptr<opencl> context, unsigned int width, unsigned int height) : imageBuffer() {
			_width = width;
			_height = height;
			addItem(context->floatImage(_width, _height));

			inLock();
			inUnlock();

			queue = context->getQueue();
		}

		~testImageGenerator() {

		}

		void setData(float * data) {
			outLock();
			cl::Event event;
			cl::size_t<3> origin, region;
			origin[0] = 0; origin[1] = 0; origin[2] = 0;
			region[0] = _width; region[1] = _height; region[2] = 1;

			queue->enqueueWriteImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, data, NULL, &event);
			event.wait();

			outUnlock();
		}

		void outUnlock() {
			imageBuffer::outUnlock();
			inLock();
			inUnlock();

		}


	private:
		
	};

}