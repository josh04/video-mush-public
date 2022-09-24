
#include "frameGrabber.hpp"

namespace mush {

	void frameGrabber::startThread() {
        try {
            gather();
        } catch (std::runtime_error& e) {
            putLog(e.what());
            release();
        }
	}

	const mush::inputEngine frameGrabber::inputEngine() const {
		return _inputEngine;
	}

}