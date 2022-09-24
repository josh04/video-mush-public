//
//  newRasteriser.hpp
//  parcore
//
//  Created by Josh McNamee on 30/06/2015.
//  Copyright (c) 2015 Josh McNamee. All rights reserved.
//

#ifndef parcore_newRasteriser_hpp
#define parcore_newRasteriser_hpp

#include <iostream>
#include <fstream>
#include <ctime>

#include "mush-core-dll.hpp"
#include "opencl.hpp"

#include "camera_path_io.hpp"
#include "camera_event_handler.hpp"
#include "camera_base.hpp"
#include "cameraConfig.hpp"
#include <azure/Framebuffer.hpp>
#include <azure/Eventable.hpp>
#include <azure/Program.hpp>

namespace mush {
	namespace raster {
        class path_drawer;
		class scene;
		class sceneTesselation;
        class sphere;
		class MUSHEXPORTS_API engine
		{
		public:
			engine();
			~engine();

            void init(const core::cameraConfigStruct& c, std::shared_ptr<raster::sceneTesselation> scene, std::shared_ptr<raster::sphere> sphere);

			void render();

			void write_camera_path(const char * path);

			std::shared_ptr<azure::Eventable> get_camera() const {
				return _camera_event;
			}

			void add_stereo_shift();
			void remove_stereo_shift();

			cl_float3 get_camera_position() const {
				auto pos = _camera->get_location();
				return{ pos.x, pos.y, pos.z };
			}
			cl_float3 get_camera_theta_phi_fov() const {
				return{ _camera->get_theta(), _camera->get_phi(), _camera->get_fov() };
			}
            
            void set_size(unsigned int width, unsigned int height) {
                _width = width;
                _height = height;
            }
            
            void update_program(std::shared_ptr<azure::Program> program, bool normal, bool light, bool tess) const;
            
            glm::mat4 get_model_matrix();

			std::shared_ptr<mush::raster::scene> _scene = nullptr;
			std::shared_ptr<mush::raster::sceneTesselation> _scene_tess = nullptr;

			std::shared_ptr<mush::camera::base> _camera = nullptr;
            std::shared_ptr<mush::camera::camera_event_handler> _camera_event = nullptr;
            std::shared_ptr<mush::raster::path_drawer> _camera_path_drawer = nullptr;
            
            std::shared_ptr<mush::raster::sphere> _sphere = nullptr;
		private:
			unsigned int _width = 0, _height = 0;
			float _stereo_distance = 0.63f;

			glm::vec3 _non_stereo_location;
		};
	}
}


#endif
