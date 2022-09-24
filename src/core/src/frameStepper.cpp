#include "frameStepper.hpp"
#include "imageProcess.hpp"

namespace mush {
	frameStepper::frameStepper() : mush::processNode(), sw(false), enabled(false) {

	}

	frameStepper::~frameStepper() {

	}

	void frameStepper::process() {
		std::unique_lock<std::mutex> lock(mut);
		cond.wait(lock, [&]() -> bool { return sw || !enabled; });

		sw = false;
	}

	void frameStepper::throw_switch() {
		sw = true;
		cond.notify_one();
	}

	void frameStepper::toggle() {
		enabled = !enabled;
		cond.notify_one();
	}

	void frameStepper::release() {
		enabled = false;
		cond.notify_all();
	}
}