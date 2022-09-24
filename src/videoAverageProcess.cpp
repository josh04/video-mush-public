
#include "videoAverageProcess.hpp"

namespace mush {

	videoAverageProcess::videoAverageProcess(int cap) : mush::singleKernelProcess("video_average"), _cap(cap) {

	}

	videoAverageProcess::~videoAverageProcess() {

	}

	void videoAverageProcess::setArgs() {
		int deduct = 0;
		if (_cap > 0) {
			if (_count < _cap) {
				_count++;
			} else {
				deduct = 1;
			}
		} else {
			_count++;
		}
		_kernel->setArg(2, _getImageMem(0));
		_kernel->setArg(3, _count);
		_kernel->setArg(4, deduct);

	}
}
