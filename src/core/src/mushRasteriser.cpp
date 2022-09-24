#include "mushRasteriser.hpp"

#include "rasteriserEngine.hpp"
#include "rasteriserSceneTesselation.hpp"
#include "cubeMapDrawer.hpp"
#include "tagInGui.hpp"
#include "sphereGL2.hpp"

namespace mush {
    rasteriser::rasteriser(const core::cameraConfigStruct& c, bool draw_sbs_stereo) : openglProcess(c.width, c.height), _config(c), _draw_sbs_stereo(draw_sbs_stereo) {
		_bg_colour = { 0.5, 1.0f, 0.5, 1.0f };
	}

	rasteriser::~rasteriser() {

	}

	void rasteriser::release() {
        thread_init();
		_cube_map_drawer = nullptr;
		_cube_map = nullptr;
		_sphere = nullptr;

		if (_engine != nullptr) {
			_engine->write_camera_path(_config.save_path);

			_engine = nullptr;
		}
		if (_depth != nullptr) {

			std::vector<cl::Memory> glObjects;
			glObjects.push_back(*_depth);
			cl::Event event;
			glFinish();
			_glException();
			queue->enqueueReleaseGLObjects(&glObjects, NULL, &event);
			event.wait();
			glFinish();
			_glException();

			_frame_depth = nullptr;
			delete _depth;
			_depth = nullptr;
		}
		if (_frame_main != nullptr) {
			_frame_main = nullptr;
		}

		release_gl_assets();
		imageProcess::release();
	}

	void rasteriser::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		opengl_init(context);

		bind_second_context();
		_engine = std::make_shared<mush::raster::engine>();

		auto scene_tess = std::make_shared<raster::sceneTesselation>();
		scene_tess->init(_config.model_path, _config.model_scale);

		if (_config.equirectangular) {
			_config.width = _config.height;
		}
        
        if (buffers.size() > 0 && buffers.begin()[0] != nullptr) {
            _input = castToImage(buffers.begin()[0]);
            
            unsigned int w, h, sz;
            _input->getParams(w, h, sz);
            
            _sphere = std::make_shared<mush::raster::sphere>(glm::vec3(0.0f, 0.0f, 0.0f), 10000000.0f, w, h);
            
            _input_texture = context->glImage(_sphere->get_texture()->getTexture(), CL_MEM_READ_WRITE, _sphere->get_texture()->getTarget(), false);
        } else {
            _sphere = nullptr;
        }

		_engine->init(_config, scene_tess, _sphere);

		create_cl_framebuffer(context, _width, _height, _depth, _frame_depth);
        //_frame_depth->setClearColor(glm::vec4(std::numeric_limits<float>::max()));
        _frame_depth->setClearColor(glm::vec4(20000.0f));

		if (_config.equirectangular) {
            _use_cube_map = true;
            _engine->_camera->set_spherical(true);
            create_cube_map();
		}
        
		unbind_second_context();

		if (_draw_sbs_stereo) {
			_left = context->floatImage(_width, _height);
			_width = _width * 2;
			_pack = context->getKernel("sbs_pack");
			setTagInGuiStereo(true);
		}

		//_flip = _copy;
        
        //_frame_depth = std::make_shared<azure::Framebuffer>();

		addItem(context->floatImage(_width, _height));
        
        _main = _frameCL;
        _frame_main = _frame;
	}
    
    void rasteriser::create_cube_map() {
        
        _cube_map = std::make_shared<azure::Framebuffer>();
        _cube_map->setTarget(GL_TEXTURE_CUBE_MAP);
        _cube_map->createTexture(GL_RGBA32F, _config.height, _config.height, GL_RGBA, GL_FLOAT, NULL);
        _cube_map->createDepthTexture();
        _cube_map_drawer = std::make_shared<raster::cubeMapDrawer>(_cube_map);
        
        _created_cube_map = true;

    }

	void rasteriser::process() {
		bool r_o = false;
		if (!has_run_once()) {
			r_o = true;
		}
        
        if (_input.get() != nullptr) {
            
            cl::Event event3;
            std::vector<cl::Memory> glObjects;
            glObjects.push_back(*_input_texture);
            
            queue->enqueueAcquireGLObjects(&glObjects, NULL, &event3);
            event3.wait();
            
            //auto ptr = _input->outLock(16000000);
            //auto ptr = _input->outLock(16000000, _input_token);
            auto ptr = _input->outLock(10);
            if (ptr != nullptr) {
                _flip->setArg(0, ptr.get_image());
                _flip->setArg(1, *_input_texture);
                
                cl::Event event;
                queue->enqueueNDRangeKernel(*_flip, cl::NullRange, cl::NDRange(_sphere->get_width(), _sphere->get_height()), cl::NullRange, NULL, &event);
                event.wait();
                
                _input->outUnlock();
            }
            
            cl::Event event2;
            queue->enqueueReleaseGLObjects(&glObjects, NULL, &event2);
            event2.wait();
            
        }

		if (_draw_sbs_stereo) {

			bind_gl();

            if (_enable_auto_camera) {
                _engine->_camera_event->frame_tick();
            } else if (_single_auto_camera_tick) {
                _engine->_camera_event->frame_tick();
                _single_auto_camera_tick = false;
            }
            
            _engine->_camera_event->move_camera();
            
			_engine->render();
			unbind_gl();

			cl::Event event;

			_copy->setArg(0, *_frameCL);
			_copy->setArg(1, *_left);

			queue->enqueueNDRangeKernel(*_copy, cl::NullRange, cl::NDRange(_width / 2, _height), cl::NullRange, NULL, &event);
			event.wait();

			bind_gl();
			_engine->add_stereo_shift();
			_engine->render();
			_engine->remove_stereo_shift();
            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            //_glException();
			unbind_gl();

			auto& buf = inLock();
			
			buf.set_camera_position(_engine->get_camera_position(), _engine->get_camera_theta_phi_fov());

			_pack->setArg(0, *_left);
			_pack->setArg(1, *_frameCL);
			_pack->setArg(2, buf.get_image());
			queue->enqueueNDRangeKernel(*_pack, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();

			inUnlock();

		} else {
			bind_gl();
            
            
            if (_enable_auto_camera) {
                _engine->_camera_event->frame_tick();
            } else if (_single_auto_camera_tick) {
                _engine->_camera_event->frame_tick();
                _single_auto_camera_tick = false;
            }
            
            _engine->_camera_event->move_camera();
            
			if (_use_cube_map) {
				draw_cube_map();
			} else {
				_engine->render();
			}
			unbind_gl();
            process_gl(postprocess_kernel::copy);
		}

		if (r_o) {
			if (_depth != nullptr) {
				std::vector<cl::Memory> glObjects;
				glObjects.push_back(*_depth);
				cl::Event event;
				glFinish();
				_glException();
				queue->enqueueAcquireGLObjects(&glObjects, NULL, &event);
				event.wait();
				glFinish();
				_glException();
			}
		}
	}

	void rasteriser::process_depth() {
		enable_depth_program();
		auto bg_colour = _frame_depth->getClearColor();
		bind_gl(false);

		glClearColor(bg_colour[0], bg_colour[1], bg_colour[2], bg_colour[3]);
		_glException();
		glClear(GL_COLOR_BUFFER_BIT);
		_glException();

		if (_use_cube_map) {
			draw_cube_map();
		} else {
			_engine->render();
		}

		unbind_gl();
		disable_depth_program();
	}

	void rasteriser::draw_cube_map() {
        if (!_created_cube_map) {
            create_cube_map();
        }
        
		//glEnable(GL_DEPTH_TEST);
		_cube_map->bind(false);
        
        
        glViewport(0, 0, _height, _height);
        _glException();
        glScissor(0, 0, _height, _height);
        _glException();
        

		_cube_map->attach(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_glException();

		float theta = _engine->_camera->get_theta();

		_engine->_camera->push_temporary_position(_engine->_camera->get_location(), theta + 0, 0, 90.0f);
		_engine->render();
		_engine->_camera->pop_temporary_position();

		_cube_map->attach(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_glException();
		_engine->_camera->push_temporary_position(_engine->_camera->get_location(), theta -90, 0, 90.0f);
		_engine->render();
		_engine->_camera->pop_temporary_position();

		_cube_map->attach(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_glException();
		_engine->_camera->push_temporary_position(_engine->_camera->get_location(), theta + 180, -90, 90.0f);
		_engine->render();
		_engine->_camera->pop_temporary_position();

		_cube_map->attach(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_glException();
		_engine->_camera->push_temporary_position(_engine->_camera->get_location(), theta + 90, 0, 90.0f);
		_engine->render();
		_engine->_camera->pop_temporary_position();

		_cube_map->attach(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_glException();
		_engine->_camera->push_temporary_position(_engine->_camera->get_location(), theta + 180, 90, 90.0f);
		_engine->render();
		_engine->_camera->pop_temporary_position();

		_cube_map->attach(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_glException();
		_engine->_camera->push_temporary_position(_engine->_camera->get_location(), theta + 180, 0, 90.0f);
		_engine->render();

		_engine->_camera->pop_temporary_position();

		_frame->bind(false);
		glViewport(0, 0, _width, _height);
		_glException();
		glScissor(0, 0, _width, _height);
		_glException();

        glDisable(GL_DEPTH_TEST);
        _glException();
        //glDisable(GL_CULL_FACE);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_glException();
        _cube_map_drawer->render();
        glEnable(GL_DEPTH_TEST);
        _glException();
	}

	std::shared_ptr<azure::Eventable> rasteriser::get_eventable() const {
		return _engine->get_camera();
	}
    
    void rasteriser::enable_depth_program() {
        _engine->_scene_tess->enable_depth_program();
        _use_depth_program = true;
        _frameCL = _depth;
        _frame = _frame_depth;
    }
    void rasteriser::disable_depth_program() {
        _engine->_scene_tess->disable_depth_program();
        _use_depth_program = false;
        _frameCL = _main;
        _frame = _frame_main;
    }
    
    void rasteriser::enable_spherical() {
        _use_cube_map = true;
        _engine->_camera->set_spherical(true);
        _engine->set_size(_height, _height);
    }
    
    void rasteriser::disable_spherical() {
        _use_cube_map = false;
        _engine->_camera->set_spherical(false);
        _engine->set_size(_width, _height);
    }
    
    void rasteriser::inUnlock() {
        if (getTagInGuiMember() && tagGui != nullptr) {
            if (_draw_sbs_stereo) {
                tagGui->copyImageIntoGuiBuffer(getTagInGuiIndex(), _getMem(next));
            } else {
                tagGui->copyImageIntoGuiBuffer(getTagInGuiIndex(), _frame_main);
            }
        }
        ringBuffer::inUnlock();
    }

    void rasteriser::enable_auto_camera() {
        _enable_auto_camera = true;
    }
    
    void rasteriser::disable_auto_camera() {
        _enable_auto_camera = false;
    }

    
}
