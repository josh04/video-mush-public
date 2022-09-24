//
//  newRasteriser.cpp
//  parcore
//
//  Created by Josh McNamee on 30/06/2015.
//  Copyright (c) 2015 Josh McNamee. All rights reserved.
//
#define _USE_MATH_DEFINES
#include <math.h>
#ifdef __APPLE__
#include <Cocoa/Cocoa.h>

//#include <CoreFoundation/CoreFoundation.h>

#include <azure/glm/gtc/matrix_transform.hpp>
#include <azure/glm/gtx/transform.hpp>
#else 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#endif
#include "rasteriserEngine.hpp"

#include <azure/Shader.hpp>
#include <azure/ShaderFactory.hpp>
#include <azure/Program.hpp>


#include "rasteriserPathDrawer.hpp"
#include "rasteriserScene.hpp"
#include "rasteriserSceneTesselation.hpp"

#include "sphereGL2.hpp"


namespace mush {
	namespace raster {

		engine::engine()
		{

		}

		engine::~engine()
		{
			_glException();
			_scene = nullptr;
			_scene_tess = nullptr;
		}

        void engine::init(const core::cameraConfigStruct& c, std::shared_ptr<raster::sceneTesselation> scene, std::shared_ptr<raster::sphere> sphere) {

            _sphere = sphere;
            
			_width = c.width;
			_height = c.height;
			//_stereo_distance = c.stereo_distance;

			//_scene = std::make_shared<scene>();
			//_scene->init(c.model_path, c.model_scale);
			_scene_tess = scene;

			_camera = std::make_shared<mush::camera::base>();
			_camera_event = std::make_shared<mush::camera::camera_event_handler>(_camera);
			_camera->move_camera({ c.position_x, c.position_y, c.position_z }, c.position_theta, c.position_phi);
            
            if (std::string(c.load_path).size() > 0) {
				if (c.autocam || c.display_cam) {
					_camera_event->load_camera_path(c.load_path, c.speed, false, c.quit_at_camera_path_end);
				}

				if (c.display_cam) {

					if (!c.autocam) {
						_camera_event->end_camera_path();
					}

						_camera_path_drawer = std::make_shared<mush::raster::path_drawer>();
						_camera_path_drawer->init(_camera_event->get_camera_path());

				}
            }
            
		}

		glm::mat4 MakeInfReversedZProjRH(float fovY_radians, float aspectWbyH, float zNear)
		{
			float f = 1.0f / tan(fovY_radians / 2.0f);
			return glm::mat4(
				f / aspectWbyH, 0.0f, 0.0f, 0.0f,
				0.0f, -f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, -1.0f,
				0.0f, 0.0f, zNear, 0.0f);
		}

		void engine::render()
		{
/*
			glViewport(0, 0, _width, _height);
            _glException();
            glScissor(0, 0, _width, _height);
            _glException();
*/
			glActiveTexture(GL_TEXTURE0);
			_glException();

			glDepthFunc(GL_GREATER);
#ifndef __APPLE__
			glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
			_glException();
			glClearDepth(0.0f);
			_glException();
#else
			glClearDepth(-1.0f);
			_glException();
#endif
			
			glClear(GL_DEPTH_BUFFER_BIT);
			_glException();

			//_camera->move_camera(parCamera->location, parCamera->theta, parCamera->phi);
			//_camera->set_fov(parCamera->fov);

			//#ifndef __APPLE__
			//    glm::mat4 Projection = glm::perspective(camera->get_fov(), camera->get_aspect(), FLT_MAX, 0.1f);
			//#else
			glm::mat4 Projection = MakeInfReversedZProjRH(_camera->get_fov() * M_PI / 180.0f, _width / (float)_height, 0.1f);
			//#endif

			//glm::mat4 Projection = camera->get_projection();

			glm::mat4 View = _camera->get_view();
            
            glm::mat4 Model = get_model_matrix();

			glm::mat4 MV         = View * Model;
			//glm::mat4 MVP        = camera->get_mvp();
			glm::mat4 MVP = Projection * View * Model;
            
            if (_camera_path_drawer != nullptr) {
				auto program = _camera_path_drawer->getProgram();
				program->use();
				program->uniformV("MVP", false, &MVP[0][0]);
				program->discard();
            }

			auto program = _scene_tess->getProgram();

			program->use();

			program->uniformV("M", false, &Model[0][0]);
			program->uniformV("V", false, &View[0][0]);
			//    _program->uniformV("MV", false, &MV[0][0]);
			program->uniformV("MVP", false, &MVP[0][0]);

			auto norm_matrix = glm::mat3(glm::transpose(glm::inverse(MV)));
			program->uniformM("NormalMatrix", false, &norm_matrix[0][0]);

            auto camera_location = _camera->get_location();
			GLfloat position[] = { camera_location[0], camera_location[1], camera_location[2], 1.0f };

			GLint location;
			if ((location = program->uniform("light_position")) < 0) { 
				throw std::runtime_error("Light position not located."); 
			}
			glUniform3fv(location, 1, position);
			_glException();

			program->uniform("TessLevelInner", 1.0f);
			program->uniform("TessLevelOuter", 1.0f);

            auto depth_program = _scene_tess->get_depth_program();
            depth_program->use();
            depth_program->uniformV("M", false, &Model[0][0]);
            depth_program->uniformV("V", false, &View[0][0]);
            //    _program->uniformV("MV", false, &MV[0][0]);
            depth_program->uniformV("MVP", false, &MVP[0][0]);
            depth_program->uniformM("NormalMatrix", false, &norm_matrix[0][0]);

			if ((location = depth_program->uniform("light_position")) < 0) {
				throw std::runtime_error("Light position not located.");
			}
            glUniform3fv(location, 1, position);
            _glException();
            depth_program->uniform("TessLevelInner", 1.0f);
            depth_program->uniform("TessLevelOuter", 1.0f);

            
            auto texture_program = _scene_tess->get_texture_program();
            texture_program->use();
            texture_program->uniformV("M", false, &Model[0][0]);
            texture_program->uniformV("V", false, &View[0][0]);
            //    _program->uniformV("MV", false, &MV[0][0]);
            texture_program->uniformV("MVP", false, &MVP[0][0]);
            texture_program->uniformM("NormalMatrix", false, &norm_matrix[0][0]);
            
            if ((location = texture_program->uniform("light_position")) < 0) {
                throw std::runtime_error("Light position not located.");
            }
            glUniform3fv(location, 1, position);
            _glException();
            texture_program->uniform("TessLevelInner", 1.0f);
            texture_program->uniform("TessLevelOuter", 1.0f);
            
			/*
			glEnable(GL_CULL_FACE);
			_glException();
			glCullFace(GL_FRONT);
			_glException();
			*/
            
            
            if (_sphere != nullptr) {
                auto program = _sphere->getProgram();
                
                program->use();
                program->uniformV("M", false, &Model[0][0]);
                program->uniformV("V", false, &View[0][0]);
                //    _program->uniformV("MV", false, &MV[0][0]);
                program->uniformV("MVP", false, &MVP[0][0]);
                
                    if ((location = program->uniform("light_position")) < 0) {
                        throw std::runtime_error("Light position not located.");
                    }
                    glUniform3fv(location, 1, position);
                    _glException();
                
                
                //update_program(_sphere->getProgram(), false, true, false);
                _sphere->render();
            }
            
			_scene_tess->attach();
			_scene_tess->render();
            
            if (_camera_path_drawer != nullptr) {
                _camera_path_drawer->attach();
                _camera_path_drawer->render();
            }

			glDepthFunc(GL_LESS);
			glClearDepth(1.0f);
			glDisable(GL_CULL_FACE);
			_glException();

		}

		void engine::add_stereo_shift() {
			_non_stereo_location = _camera->get_location();

			glm::vec3 dir = _camera->get_gaze();
			glm::vec3 tup = _camera->get_up();
			glm::vec3 right = glm::cross(dir, tup);
			_camera->add_location(right * _stereo_distance);

		}
		void engine::remove_stereo_shift() {
			_camera->move_camera(_non_stereo_location, _camera->get_theta(), _camera->get_phi());
		}

		void engine::write_camera_path(const char * path) {
			if (_camera_event != nullptr) {
				_camera_event->write_camera_path(path);
			}
		}
        
        void engine::update_program(std::shared_ptr<azure::Program> program, bool normal, bool light, bool tess) const {
            /*
            program->use();
            program->uniformV("M", false, &Model[0][0]);
            program->uniformV("V", false, &View[0][0]);
            //    _program->uniformV("MV", false, &MV[0][0]);
            program->uniformV("MVP", false, &MVP[0][0]);
            
            if (normal) {
                program->uniformM("NormalMatrix", false, &norm_matrix[0][0]);
            }
            if (light) {
                if ((location = texture_program->uniform("light_position")) < 0) {
                    throw std::runtime_error("Light position not located.");
                }
                glUniform3fv(location, 1, position);
                _glException();
            }
            if (tess) {
                program->uniform("TessLevelInner", 1.0f);
                program->uniform("TessLevelOuter", 1.0f);
            }
            */
        }
        
        glm::mat4 engine::get_model_matrix() {
            static int tick = 0;
            
            glm::mat4 Model = glm::mat4(1.0f);
            
            //Model = glm::rotate(Model, (float)tick, glm::vec3{0,1,0});
            tick++;
            return Model;
        }

	}

}
