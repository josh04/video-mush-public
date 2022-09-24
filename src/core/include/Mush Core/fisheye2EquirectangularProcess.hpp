#ifndef FISHEYE_2_EQUIRECTANGULAR_PROCESS
#define FISHEYE_2_EQUIRECTANGULAR_PROCESS

#include "mush-core-dll.hpp"
#include "singleKernelProcess.hpp"
#include <azure/eventable.hpp>

namespace mush {
    class MUSHEXPORTS_API fisheye2EquirectangularProcess : public singleKernelProcess, public azure::Eventable {
	public:
        fisheye2EquirectangularProcess(float fov, float x_centre, float y_centre);

        ~fisheye2EquirectangularProcess();

        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;

        void setArgs() override;
        
        bool event(std::shared_ptr<azure::Event> event) override;
	private:
		float _fov;
        float _x_centre = 0.497f, _y_centre = 0.52f;
	};
}

#endif
