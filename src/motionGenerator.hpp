#ifndef MOTION_GENERATOR_HPP
#define MOTION_GENERATOR_HPP

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace cl {
	class Kernel;
	class CommandQueue;
}

namespace mush {
	namespace camera {
		class base;
		class parBase;
        class camera_event_handler;
	}
}

class motionGenerator : public mush::imageProcess {
public:
	motionGenerator(bool is_360, const char * camera_path, float speed_factor);
	~motionGenerator();

	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;

	void process() override;

private:
	//cl::Kernel * motion_reprojection = nullptr;

	bool _is_360 = false;

	cl_float4 * _depth_host = nullptr;
	cl_float4 * _motion_host = nullptr;

	mush::registerContainer<mush::imageBuffer> _depth;
    
    std::shared_ptr<mush::camera::base> _camera = nullptr;
	std::shared_ptr<mush::camera::parBase> _par = nullptr;
    std::shared_ptr<mush::camera::camera_event_handler> _event_handler = nullptr;
    
    std::string _camera_path = "";
    float _speed_factor = 1.0f;
    
    float stereo_offset = 0.63f;
};

#endif
