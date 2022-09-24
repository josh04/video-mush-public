///
//  motionGenerator.cpp
//  video-mush
//
//  Created by Josh McNamee on 10/02/2017.
//
//
#define _USE_MATH_DEFINES
#include <math.h>
#include <Mush Core/opencl.hpp>

#include <Mush Core/camera_base.hpp>
#include <Mush Core/camera_event_handler.hpp>
#ifdef _WIN32
#include <PARtner/cameraPar.hpp>
#include <PARtner/cameraPar360.hpp>
#endif
#ifdef __APPLE__
#include <ParFramework/cameraPar.hpp>
#include <ParFramework/cameraPar360.hpp>
#endif

#include "motionGenerator.hpp"
#include "../../PARCore/src/Misc/Ray.h"


motionGenerator::motionGenerator(bool is_360, const char * camera_path, float speed_factor) : mush::imageProcess(), _is_360(is_360), _camera_path(camera_path), _speed_factor(speed_factor) {

}

motionGenerator::~motionGenerator() {

}

void motionGenerator::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
	//assert(buffers.size() == 1);

	//motion_reprojection = context->getKernel("motion_reprojection_demo");

	_depth = castToImage(buffers.begin()[0]);
	_depth->getParams(_width, _height, _size);

    _motion_host = (cl_float4 *)context->hostWriteBuffer(_width * _height * sizeof(cl_float4));
    _depth_host = (cl_float4 *)context->hostWriteBuffer(_width * _height * sizeof(cl_float4));
    
    _camera = std::make_shared<mush::camera::base>();
	if (_is_360) {
		_par = std::make_shared<mush::camera::par360>();
	} else {
		_par = std::make_shared<mush::camera::par>();
	}
    
    _par->width = _width;
    _par->height = _height;
    
    _event_handler = std::make_shared<mush::camera::camera_event_handler>(_camera);
    
    _event_handler->load_camera_path(_camera_path, _speed_factor, true);

	addItem(context->floatImage(_width, _height));

	queue = context->getQueue();
}

void motionGenerator::process() {
	inLock();

	cl::Event event;

	auto image = _depth->outLock();
	if (image == nullptr) {
		release();
		return;
	}
    
    _event_handler->frame_tick();
    
    
    if (_is_360) {
        _par->move_camera(_camera->get_location(), _camera->get_theta()+M_PI_2, 0.0f);
    } else {
        _par->move_camera(_camera->get_location(), _camera->get_theta() + M_PI_2, _camera->get_phi());
    }
    //_par->set_fov(_camera->get_fov());
    //_par->set_additional_displacement(_camera->get_additional_displacement());
    //_par->set_additional_shift_matrix(_camera->get_additional_shift_matrix());
    
    _par->set_camera_motion_keyframe();
    
    auto r = _par->get_right();
    
    if (_is_360) {
    r.RotateY(-M_PI_2);
    }
    //r.Normalize();
    
    //std::cout << r.x << " " << r.y << " " << r.z << std::endl;
    
    float second_camera_x_diff = r.x * stereo_offset;
    float second_camera_y_diff = r.y * stereo_offset;
    float second_camera_z_diff = r.z * stereo_offset;
    
    float second_camera_theta_diff = 0.0f;
    float second_camera_phi_diff = 0.0f;
    float second_camera_fov_diff = 0.0f;
    
    auto r3 = _par->get_location();
    //std::cout << r3.x << " " << r3.y << " " << r3.z << std::endl;
    if (_is_360) {
        _par->move_camera(_par->get_location() + glm::vec3{ second_camera_x_diff, second_camera_y_diff, second_camera_z_diff }, _par->get_theta() + second_camera_theta_diff, 0.0f);
    } else {
        _par->move_camera(_par->get_location() + glm::vec3{ second_camera_x_diff, second_camera_y_diff, second_camera_z_diff }, _par->get_theta() + second_camera_theta_diff, _par->get_phi() + second_camera_phi_diff);
    }
    
    //_par->set_fov(_par->get_fov() + second_camera_fov_diff);
    //camera->set_additional_displacement(main_camera->get_additional_displacement());
    //camera->set_additional_shift_matrix(main_camera->get_additional_shift_matrix());
    _par->get_right();
    auto r2 = _par->get_location();
    //std::cout << r2.x << " " << r2.y << " " << r2.z << std::endl;
    
    
	cl::size_t<3> origin, region;
	origin[0] = 0; origin[1] = 0; origin[2] = 0;
	region[0] = _width; region[1] = _height; region[2] = 1;

	queue->enqueueReadImage(image.get_image(), CL_TRUE, origin, region, 0, 0, _depth_host, NULL, &event);
	event.wait();

	for (int j = 0; j < _height; ++j) {
		for (int i = 0; i < _width; ++i) {
			auto d = _depth_host[j * _width + i];
			Ray r;
            
            if (_is_360) {
                _par->getRay((float)i + 0.5f, (float)j + 0.5f, 0.0f, 0.0f, r);
            } else {
                _par->getRay((float)i + 0.5f, (float)j + 0.5f, 0.0f, 0.0f, r);
            }
            
			double u, v, de;
			_par->get_motion_u_v(r, d.s[0], u, v, de);

            
            if (_is_360) {
                u = (float)i + 0.5f - u;
                v = (float)j + 0.5f - v;
                _motion_host[j * _width + i] = { (float)u, (float)v, (float)de, 1.0f };
            } else {
                u = (float)i + 0.5f - u;
                v = (float)j + 0.5f - v;
                _motion_host[j * _width + i] = { (float)-u, (float)v, (float)de, 1.0f };
            }
		}
	}

	queue->enqueueWriteImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, _motion_host, NULL, &event);
	event.wait();

	_depth->outUnlock();
	inUnlock();
}
