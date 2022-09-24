
#include "dualisoWeight.hpp"

#include <Mush Core/tagInGui.hpp>
#include <Mush Core/opencl.hpp>

namespace mush {
	dualiso_weight::dualiso_weight(int mod) : mush::imageProcess(), azure::Eventable(), _mod(mod) {

	}


	void dualiso_weight::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		assert(buffers.size() == 1);

		_weight = context->getKernel("dualiso_weight");

		buffer = castToImage(buffers.begin()[0]);

		buffer->getParams(_width, _height, _size);

		addItem(context->floatImage(_width, _height));

		_weight->setArg(1, _getImageMem(0));
		_weight->setArg(2, _mod);
		queue = context->getQueue();
	}

	void dualiso_weight::process() {
		inLock();
		auto input = buffer->outLock();
		if (input == nullptr) {
			release();
			return;
		}

		cl::Event event;

		_weight->setArg(0, input.get_image());
		_weight->setArg(3, _wb_red);
		_weight->setArg(4, _wb_blue);

		queue->enqueueNDRangeKernel(*_weight, cl::NullRange, cl::NDRange(_width / 2, _height / 2), cl::NullRange, NULL, &event);
		event.wait();


		buffer->outUnlock();
		inUnlock();
	}

	bool dualiso_weight::event(std::shared_ptr<azure::Event> event) {

		if (event->isType("whitePointReport")) {
			_wb_red = 1.0f / event->getAttribute<float>("red");
			_wb_blue = 1.0f / event->getAttribute<float>("blue");
		}

		return false;
	}

	void dualiso_weight::inUnlock() {
		if (getTagInGuiMember() && tagGui != nullptr) {
			_weight_display->setArg(0, _getMem(next).get_buffer());
			cl::Event event;
			queue->enqueueNDRangeKernel(*_weight_display, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();

			mush::buffer temp{ _weight_image };
			if (_getMem(next).has_camera_position()) {
				temp.set_camera_position(_getMem(next).get_camera_position(), _getMem(next).get_theta_phi_fov());
			}

			tagGui->copyImageIntoGuiBuffer(getTagInGuiIndex(), temp);
		}
		ringBuffer::inUnlock();
	}

	void dualiso_weight::guiTag(int i, tagInGui * tag) {
		if (tag != nullptr) {
			if (_weight_image == nullptr) {
				create_temp_image(tag->getContext());
			}
		}
		guiAccessible::guiTag(i, tag);
	}

	void dualiso_weight::create_temp_image(std::shared_ptr<opencl> context) {
		std::lock_guard<std::mutex> lock(_lock);
		if (_weight_image == nullptr) {
			queue = context->getQueue();
			_weight_image = context->floatImage(_width, _height);

			_weight_display = context->getKernel("weight_display");

			_weight_display->setArg(1, *_weight_image);
		}
	}

}
