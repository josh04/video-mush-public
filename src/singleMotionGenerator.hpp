//
//  singleMotionGenerator.hpp
//  video-mush
//
//  Created by Josh McNamee on 09/05/2017.
//
//

#ifndef singleMotionGenerator_hpp
#define singleMotionGenerator_hpp

#include <azure/glm/glm.hpp>

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace cl {
    class Kernel;
    class CommandQueue;
}

namespace mush {
    namespace camera {
        class base;
		class par;
        class parBase;
        class camera_event_handler;
    }
}

class singleMotionGenerator : public mush::imageProcess {
public:
    singleMotionGenerator(bool is_360, float x, float y, float z, float theta, float phi, float fov, bool source_is_spherical);
    ~singleMotionGenerator();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
	void process() override;

	void set_camera(std::shared_ptr<mush::camera::base> cam);
    void set_camera_orig(glm::vec3 loc, float theta, float phi, float fov);
private:
    cl::Kernel * generate_motion_from_depth = nullptr;
    
    float _orig_x = 0.0f, _orig_y = 0.0f, _orig_z = 0.0f;
    float _orig_theta = 0.0f, _orig_phi = 0.0f, _orig_fov = 75.0f;

	float _new_x = 0.0f, _new_y = 0.0f, _new_z = 0.0f;
	float _new_theta = 0.0f, _new_phi = 0.0f, _new_fov = 75.0f;
    
    bool _is_360 = false;
    
    cl_float4 * _depth_host = nullptr;
    cl_float4 * _motion_host = nullptr;
    
    mush::registerContainer<mush::imageBuffer> _depth;
    
    std::shared_ptr<mush::camera::parBase> _par = nullptr;
    
    float stereo_offset = 0.63f;

	bool _source_is_spherical;
};


#endif /* singleMotionGenerator_hpp */
