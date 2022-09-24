#include <algorithm>
using std::min;
using std::max;
#include <memory>

#include <azure/events.hpp>
#include <azure/event.hpp>
#include <azure/eventkey.hpp>

#include <Mush Core/opencl.hpp>
#include "whitePointProcess.hpp"
#include "debayerProcess.hpp"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace mush {
	bool whitePointProcess::event(std::shared_ptr<azure::Event> event) {
		if (event->isType("whitePoint")) {
			float x = event->getAttribute<float>("x") / 50.0;
			float y = event->getAttribute<float>("y") / 50.0;

			_point = { x, 1.0f, y, 1.0f };
            
            setArgs();

			auto event = std::unique_ptr<azure::Event>(new azure::Event("whitePointReport", 0));
			event->setAttribute<float>("red", _point.s0);
			event->setAttribute<float>("green", _point.s1);
			event->setAttribute<float>("blue", _point.s2);
			azure::Events::Push(std::move(event));
		} else if (event->isType("whitePointFromDebayer")) {
			auto debayer = std::dynamic_pointer_cast<mush::debayerProcess>(_buffer.get());
			_point = debayer->getWhitePoint();

			_point.s0 = std::min(std::max(_point.s0, 0.0f), 10.0f);
			_point.s2 = std::min(std::max(_point.s2, 0.0f), 10.0f);
            
            setArgs();
            
			auto event = std::unique_ptr<azure::Event>(new azure::Event("whitePointReport", 0));
			event->setAttribute<float>("red", _point.s0);
			event->setAttribute<float>("green", _point.s1);
			event->setAttribute<float>("blue", _point.s2);
			azure::Events::Push(std::move(event));
        } else if (event->isType("keyDown")) {
            auto key = event->getAttribute<azure::Key>("key");
            if (key == azure::Key::l) {
                clamp += 0.1f;
                
                std::stringstream strm;
                strm << "Clamp set to " << clamp << ".";
                putLog(strm.str());
            } else if (key == azure::Key::k) {
                clamp -= 0.1f;
                std::stringstream strm;
                strm << "Clamp set to " << clamp << ".";
                putLog(strm.str());
            }
        }
		return false;
	}
    
    void whitePointProcess::setArgs() {
        _kernel->setArg(2, _point);
        _kernel->setArg(3, col1);
        _kernel->setArg(4, col2);
        _kernel->setArg(5, col3);
        _kernel->setArg(6, (int)_dual_iso);
        _kernel->setArg(7, clamp);
    }
}
