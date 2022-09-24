#ifndef MUSH_RASTERISER_360_HPP
#define MUSH_RASTERISER_360_HPP

#include "mushRasteriser.hpp"
#include "rasteriserEngine.hpp"

#include "cubeMapDrawer.hpp"

namespace mush {
	class rasteriser360 : public rasteriser {
	public:
		rasteriser360(const core::cameraConfigStruct& c, bool draw_sbs_stereo) : rasteriser(c, draw_sbs_stereo) {

		}

		~rasteriser360() {

		}

		void release() override {

			_cube_map_drawer = nullptr;
			_cube_map = nullptr;
			rasteriser::release();
		}

		void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>>& buffers) override {

            _config.width = _config.height;
			rasteriser::init(context, buffers);

			bind_second_context();

			_cube_map = std::make_shared<azure::Framebuffer>();
			_cube_map->setTarget(GL_TEXTURE_CUBE_MAP);
			_cube_map->createTexture(GL_RGBA32F, _config.height, _config.height, GL_RGBA, GL_FLOAT, NULL);
			_cube_map->createDepthTexture();
			_cube_map_drawer = std::make_shared<raster::cubeMapDrawer>(_cube_map);

			unbind_second_context();

		}

		void process() override {
			if (_draw_sbs_stereo) {
				/*
				bind_gl();

				_engine->render(true);
				unbind_gl();

				cl::Event event;

				_copy->setArg(0, *_frameCL);
				_copy->setArg(1, *_left);

				queue->enqueueNDRangeKernel(*_copy, cl::NullRange, cl::NDRange(_width / 2, _height), cl::NullRange, NULL, &event);
				event.wait();

				bind_gl();
				_engine->add_stereo_shift();
				_engine->render(false);
				_engine->remove_stereo_shift();
				unbind_gl();

				auto& buf = inLock();

				buf.set_camera_position(_engine->get_camera_position(), _engine->get_camera_theta_phi_fov());

				_pack->setArg(0, *_left);
				_pack->setArg(1, *_frameCL);
				_pack->setArg(2, buf.get_image());
				queue->enqueueNDRangeKernel(*_pack, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
				event.wait();

				inUnlock();
				*/
			} else {
				bind_gl();
				glEnable(GL_DEPTH_TEST);
				_cube_map->bind(false);
                
                _cube_map->attach(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                _glException();
                _engine->_camera->move_camera(_engine->_camera->get_location(), 0, 0);
                _engine->_camera->set_fov(90.0f);
                _engine->render(true);

                _cube_map->attach(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                _glException();
                _engine->_camera->move_camera(_engine->_camera->get_location(), -90, 0);
				_engine->render(false);

                _cube_map->attach(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                _glException();
                _engine->_camera->move_camera(_engine->_camera->get_location(), 180, -90);
				_engine->render(false);

                _cube_map->attach(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                _glException();
                _engine->_camera->move_camera(_engine->_camera->get_location(), 90, 0);
				_engine->render(false);

                _cube_map->attach(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                _glException();
                _engine->_camera->move_camera(_engine->_camera->get_location(), 180, 90);
				_engine->render(false);

                _cube_map->attach(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                _glException();
                _engine->_camera->move_camera(_engine->_camera->get_location(), 180, 0);
				_engine->render(false);

                _frame->bind(false);
                glViewport(0, 0, _width, _height);
                _glException();
                glScissor(0, 0, _width, _height);
                _glException();

				glDisable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE);
				_glException();
				_cube_map_drawer->render();

				unbind_gl();
				process_gl();
			}
		}

	private:
		std::shared_ptr<azure::Framebuffer> _cube_map = nullptr;
		std::shared_ptr<raster::cubeMapDrawer> _cube_map_drawer = nullptr;
        
	};
}


#endif


