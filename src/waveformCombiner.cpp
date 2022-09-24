
#include <Mush Core/opencl.hpp>
#include <Mush Core/tagInGui.hpp>
#include <Mush Core/gui.hpp>
#include "waveformCombiner.hpp"
#include "waveformProcess.hpp"

waveformCombiner::waveformCombiner()
	: mush::imageProcess() {
	tagInGuiUseExposure = false;
}

void waveformCombiner::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {

	waveform_combiner = context->getKernel("waveform_combiner");

	if (buffers.size() < 1) {
		putLog("Waveform combiner kernel: no buffers");
	}

	number_of_waveforms = buffers.size();

	int i = 0;
	for (auto buffer : buffers) {
		auto buf = std::dynamic_pointer_cast<mush::imageBuffer>(buffer);
		if (buf != nullptr) {
            this->buffers.push_back(mush::registerContainer<mush::ringBuffer>(buf));
			if (i == 0) {
				buf->getParams(_width, _height, _size);
			}
		}
		++i;
	}

	if (this->buffers.size() < 1) {
		putLog("Bad buffers at waveform comb");
	}

	addItem(context->floatImage(_width, _height));


	waveform_combiner->setArg(1, _getImageMem(0));

	queue = context->getQueue();
}

void waveformCombiner::process() {
	if (tagGui != nullptr) {
		const float exp = tagGui->gui->getExposure(0);
		for (int i = 0; i < number_of_waveforms; ++i) {
			std::dynamic_pointer_cast<waveformProcess>(buffers[i].get())->setExposure(exp);
		}
	}
	inLock();

	cl::Event event;
	for (int i = 0; i < number_of_waveforms; ++i) {
		auto input = buffers[i]->outLock();
		if (input == nullptr) {
			release();
			return;
		}
		waveform_combiner->setArg(0, input.get_image());
		waveform_combiner->setArg(2, i);
		queue->enqueueNDRangeKernel(*waveform_combiner, cl::NullRange, cl::NDRange(_width / number_of_waveforms, _height), cl::NullRange, NULL, &event);
		event.wait();
		buffers[i]->outUnlock();
	}

	inUnlock();
}
