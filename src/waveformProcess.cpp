

#include <Mush Core/tagInGui.hpp>
#include <Mush Core/gui.hpp>
#include <Mush Core/opencl.hpp>

#include "waveformProcess.hpp"

waveformProcess::waveformProcess(_waveform_channel ch)
	: mush::imageProcess(),
	_ch(ch) {
	tagInGuiUseExposure = false;
}

void waveformProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
	assert(buffers.size() == 1);
	if (_ch == _waveform_channel::luma) {
		waveform_count = context->getKernel("waveform_count_luma");
	} else {
		waveform_count = context->getKernel("waveform_count");
	}
	waveform_draw = context->getKernel("waveform_draw");
	waveform_clear = context->getKernel("waveform_clear");

	buffer = castToImage(buffers.begin()[0]);

	buffer->getParams(inWidth, inHeight, _size);

	luma_bins = inHeight;
	width_bins = (unsigned int)ceil(inWidth);

	_width = width_bins;
	_height = luma_bins;

	waveform_counts = context->buffer(sizeof(cl_uint)*width_bins*luma_bins);
	get_waveform_counts = (cl_int*)context->hostReadBuffer(sizeof(cl_uint)*width_bins*luma_bins);

	addItem(context->floatImage(width_bins, luma_bins));



	queue = context->getQueue();

}

void waveformProcess::process() {
	if (tagGui != nullptr) {
		setExposure(tagGui->gui->getExposure(0));
	}

	inLock();
	auto input = buffer->outLock();
	if (input == nullptr) {
		release();
		return;
	}
	waveform_count->setArg(0, input.get_image());

	cl::Event event;

	waveform_clear->setArg(0, *waveform_counts);
	waveform_clear->setArg(1, _getImageMem(0));
	waveform_clear->setArg(2, width_bins);

	queue->enqueueNDRangeKernel(*waveform_clear, cl::NullRange, cl::NDRange(width_bins, luma_bins), cl::NullRange, NULL, &event);
	event.wait();

	waveform_count->setArg(1, *waveform_counts);
	waveform_count->setArg(2, width_bins);
	waveform_count->setArg(3, luma_bins);
	waveform_count->setArg(4, _ch);
	waveform_count->setArg(5, 1.0f);
	queue->enqueueNDRangeKernel(*waveform_count, cl::NullRange, cl::NDRange(inWidth, inHeight), cl::NullRange, NULL, &event);
	event.wait();


	queue->enqueueReadBuffer(*waveform_counts, CL_TRUE, 0, sizeof(cl_uint)*width_bins*luma_bins, get_waveform_counts, NULL, &event);
	event.wait();

	const int sz = width_bins*luma_bins;
	unsigned int max = 0;
	for (int i = 0; i < sz; ++i) {
		const unsigned int get = get_waveform_counts[i];
		if (get > max) {
			max = get;
		}
	}



	waveform_draw->setArg(0, *waveform_counts);
	waveform_draw->setArg(1, _getImageMem(0));
	waveform_draw->setArg(2, max);
	waveform_draw->setArg(3, _ch);
	waveform_draw->setArg(4, luma_bins);
	queue->enqueueNDRangeKernel(*waveform_draw, cl::NullRange, cl::NDRange(width_bins, luma_bins), cl::NullRange, NULL, &event);
	event.wait();

	buffer->outUnlock();
	inUnlock();
}

void waveformProcess::setExposure(const float e) {
	const float exponent = powf(2.0f, e);
	waveform_count->setArg(5, exponent);
}
