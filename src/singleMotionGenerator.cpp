//
//  singleMotionGenerator.cpp
//  video-mush
//
//  Created by Josh McNamee on 09/05/2017.
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

#include "singleMotionGenerator.hpp"
#include "../../PARCore/src/Misc/Ray.h"


singleMotionGenerator::singleMotionGenerator(bool is_360, float x, float y, float z, float theta, float phi, float fov, bool source_is_spherical) : mush::imageProcess(), _is_360(is_360), _orig_x(x), _orig_y(y), _orig_z(z), _orig_theta(theta), _orig_phi(phi), _orig_fov(fov), _source_is_spherical(source_is_spherical) {
    
}

singleMotionGenerator::~singleMotionGenerator() {
    
}

void singleMotionGenerator::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    //assert(buffers.size() == 1);
    if (_is_360) {
		generate_motion_from_depth = context->getKernel("generate_motion_from_depth_360");
	} else {
		generate_motion_from_depth = context->getKernel("generate_motion_from_depth");
	}

    _depth = castToImage(buffers.begin()[0]);
    _depth->getParams(_width, _height, _size);
    
    _motion_host = (cl_float4 *)context->hostWriteBuffer(_width * _height * sizeof(cl_float4));
    _depth_host = (cl_float4 *)context->hostWriteBuffer(_width * _height * sizeof(cl_float4));
    
    if (_is_360) {
        _par = std::make_shared<mush::camera::par360>();
    } else {
        _par = std::make_shared<mush::camera::par>();
    }
    
    _par->width = _width;
    _par->height = _height;
    
    addItem(context->floatImage(_width, _height));
    
    queue = context->getQueue();
}

void singleMotionGenerator::process() {
    inLock();
    
    cl::Event event;
    
    auto image = _depth->outLock();
    if (image == nullptr) {
        release();
        return;
    }
    
    if (_is_360) {
        _par->move_camera({_orig_x, _orig_y, _orig_z}, _orig_theta, 0.0f);
        _par->set_fov(_orig_fov);
    } else {
        _par->move_camera({_orig_x, _orig_y, _orig_z}, _orig_theta, _orig_phi);
        _par->set_fov(_orig_fov);
    }
    //_par->set_fov(_camera->get_fov());
    //_par->set_additional_displacement(_camera->get_additional_displacement());
    //_par->set_additional_shift_matrix(_camera->get_additional_shift_matrix());
    
    _par->set_camera_motion_keyframe();
    
    auto r = _par->get_right();
    
    /*
    if (_is_360) {
        r.RotateY(-M_PI_2);
    }
     */
    //r.Normalize();
    
    //std::cout << r.x << " " << r.y << " " << r.z << std::endl;
    /*
    float second_camera_x_diff = r.x * stereo_offset;
    float second_camera_y_diff = r.y * stereo_offset;
    float second_camera_z_diff = r.z * stereo_offset;
    
    float second_camera_theta_diff = 0.0f;
    float second_camera_phi_diff = 0.0f;
    float second_camera_fov_diff = 0.0f;
     */
    auto r3 = _par->get_location();
    //std::cout << r3.x << " " << r3.y << " " << r3.z << std::endl;
    if (_is_360) {
        _par->move_camera({_new_x, _new_y, _new_z}, _new_theta, 0.0f);
        _par->set_fov(_new_fov);
    } else {
        _par->move_camera({ _new_x, _new_y, _new_z}, _new_theta, _new_phi);
		_par->set_fov(_new_fov);
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

	generate_motion_from_depth->setArg(0, image.get_image());
	generate_motion_from_depth->setArg(1, _getImageMem(0));
	auto loc = _par->get_location();
	cl_float4 cam_loc = { loc.x, loc.y, loc.z, 0.0f };
	generate_motion_from_depth->setArg(2, cam_loc);
    
    if (_is_360) {
        auto p = std::dynamic_pointer_cast<mush::camera::par360>(_par);
        
        generate_motion_from_depth->setArg(3, p->x_mul);
        generate_motion_from_depth->setArg(4, p->x_add);
        generate_motion_from_depth->setArg(5, p->y_mul);
        
        cl_float4 motion_translate = { _par->motion.translate.x, _par->motion.translate.y, _par->motion.translate.z, 0.0 };
        generate_motion_from_depth->setArg(6, motion_translate);
        generate_motion_from_depth->setArg(7, _par->motion.oldTheta);
        generate_motion_from_depth->setArg(8, _par->motion.oldPhi);
    } else {
        
        auto p = std::dynamic_pointer_cast<mush::camera::par>(_par);
        auto top_left_vec3 = p->get_top_left();
        cl_float4 top_left = { top_left_vec3.x, top_left_vec3.y, top_left_vec3.z, 0.0f };
        generate_motion_from_depth->setArg(3, top_left);
        auto dx_vec3 = p->get_dx();
        cl_float4 dx = { dx_vec3.x, dx_vec3.y, dx_vec3.z, 0.0f };
        generate_motion_from_depth->setArg(4, dx);
        auto dy_vec3 = p->get_dy();
        cl_float4 dy = { dy_vec3.x, dy_vec3.y, dy_vec3.z, 0.0f };
        generate_motion_from_depth->setArg(5, dy);

        cl_float4 motion_translate = { _par->motion.translate.x, _par->motion.translate.y, _par->motion.translate.z, 0.0 };
        generate_motion_from_depth->setArg(6, motion_translate);
        generate_motion_from_depth->setArg(7, _par->motion.oldTheta);
        generate_motion_from_depth->setArg(8, _par->motion.oldPhi);
        generate_motion_from_depth->setArg(9, _par->cwidth);
        generate_motion_from_depth->setArg(10, _par->cheight);
		generate_motion_from_depth->setArg(11, (int)_source_is_spherical);
    }

	queue->enqueueNDRangeKernel(*generate_motion_from_depth, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
	event.wait();

	/*
    queue->enqueueReadImage(*image, CL_TRUE, origin, region, 0, 0, _depth_host, NULL, &event);
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
                _motion_host[j * _width + i] = { (float)u, (float)v, (float)de, 1.0f };
            }
        }
    }
    
    queue->enqueueWriteImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, _motion_host, NULL, &event);
    event.wait();
	*/
    
    _depth->outUnlock();
    inUnlock();
}

void singleMotionGenerator::set_camera_orig(glm::vec3 loc, float theta, float phi, float fov) {
    _orig_x = loc.x;
    _orig_y = loc.y;
    _orig_z = loc.z;
    _orig_theta = theta;
    _orig_phi = phi;
    _orig_fov = fov;
    
}
void singleMotionGenerator::set_camera(std::shared_ptr<mush::camera::base> cam) {
	auto loc = cam->get_location();

	_new_x = loc.x;
	_new_y = loc.y;
	_new_z = loc.z;
	_new_theta = cam->get_theta();
	_new_phi = cam->get_phi();
	_new_fov = cam->get_fov();

}
