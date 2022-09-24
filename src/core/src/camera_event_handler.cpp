//
//  camera_event_handler.cpp
//  video-mush
//
//  Created by Josh McNamee on 09/12/2016.
//
//

#include "camera_event_handler.hpp"
#include <memory>
#include <sstream>
#include <atomic>
#define _USE_MATH_DEFINES
#include <math.h>
#include "mushLog.hpp"

#include <azure/eventtypes.hpp>

namespace mush {
    namespace camera {
        
        camera_event_handler::camera_event_handler(std::shared_ptr<base> camera) : azure::Eventable(), _camera(camera), _previous_time(std::chrono::high_resolution_clock::now()) {
            _active = true;
            _mouse_active = true;
            _tick = 0;
        }
        
        camera_event_handler::~camera_event_handler() {
            
        }
        
        void camera_event_handler::frame_tick() {
            _tick++;

			if (_camera_path != nullptr) {
				if (!_camera_path->is_finished()) {
					auto mov = _camera_path->get_next_camera_position();
					_camera->move_camera(mov.location, mov.theta, mov.phi);
					_camera->set_fov(mov.fov);
					_camera->set_aspect(mov.aspect);
                } else {
                    if (_quit_at_end) {
                        azure::Events::Push(std::unique_ptr<azure::Event>(new azure::QuitEvent()));
                    }
                }
			}
        }

		bool camera_event_handler::camera_path_finished() const {
			if (_camera_path != nullptr) {
				return _camera_path->is_finished();
			} else {
				return true;
			}
		}
        
        bool camera_event_handler::event(std::shared_ptr<azure::Event> event) {
			if (!_active) {
				return false;
			}

			std::unique_lock<std::mutex> lock(_camera_mutex);

            if (event->isType("keyDown")) {
                switch (event->getAttribute<azure::Key>("key")) {
                        /*case azure::Key::Plus:
                         if (scene->objSel < scene->models.size() - 1) {
                         scene->objSel++;
                         }
                         strm << "Selected: " << scene->objSel;
                         putLog(strm.str());
                         break;
                         case azure::Key::Minus:
                         if (scene->objSel > -2) {
                         scene->objSel--;
                         }
                         strm << "Selected: " << scene->objSel;
                         putLog(strm.str());
                         break;*/
					case azure::Key::Up:
                    case azure::Key::w:
                        _w_pressed = true;
                        break;
					case azure::Key::Down:
                    case azure::Key::s:
                        _s_pressed = true;
                        break;
					case azure::Key::Left:
                    case azure::Key::a:
                        _a_pressed = true;
                        break;
					case azure::Key::Right:
                    case azure::Key::d:
                        _d_pressed = true;
                        break;
					case azure::Key::Home:
                    case azure::Key::e:
                        _e_pressed = true;
                        break;
					case azure::Key::End:
                    case azure::Key::x:
                        _x_pressed = true;
                        break;
                    case azure::Key::o:
                        _o_pressed = true;
                        break;
                    case azure::Key::p:
                        _p_pressed = true;
                        break;
                    case azure::Key::Backspace:
                        if (_shift_pressed) {
                            _reset = true;
                        }
                        break;
                    case azure::Key::LShift:
                    case azure::Key::RShift:
                        _shift_pressed = true;
                        break;
                    case azure::Key::LCtrl:
                    case azure::Key::RCtrl:
                        _ctrl_pressed = true;
                        break;
                    case azure::Key::Space:
                    {
                        _saved_camera_positions.push_back({ _tick, {_camera->get_location(), _camera->get_theta(), _camera->get_phi(), _camera->get_fov(), _camera->get_aspect()} });
                        std::stringstream strm;
						strm << "Recorded camera position #" << _saved_camera_positions.size() << std::endl;
						auto& b = _saved_camera_positions.back();
						strm << "x: " << b.camera.location.x << " y: " << b.camera.location.y << " z: " << b.camera.location.z << std::endl;
						strm << "theta: " << b.camera.theta << " phi: " << b.camera.phi << " fov: " << b.camera.fov << " aspect: " << b.camera.aspect;
                        putLog(strm.str());
                    }
                        break;
					case azure::Key::y:
					{
						std::stringstream strm;
						auto pos = _camera->get_location();
						auto theta = _camera->get_theta();
						auto phi = _camera->get_phi();
						auto fov = _camera->get_fov();
						auto aspect = _camera->get_aspect();
						strm << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z << " theta: " << theta << " phi: " << phi;
						putLog(strm.str());
						std::stringstream strm2;
						strm2 << "fov: " << fov << " aspect: " << aspect;
						putLog(strm2.str());
					}
					break;
                    default:
                        break;
                }
            } else if(event->isType("keyUp")) {
                switch (event->getAttribute<azure::Key>("key")) {
					case azure::Key::Up:
                    case azure::Key::w:
                        _w_pressed = false;
                        break;
					case azure::Key::Down:
                    case azure::Key::s:
                        _s_pressed = false;
                        break;
					case azure::Key::Left:
                    case azure::Key::a:
                        _a_pressed = false;
                        break;
					case azure::Key::Right:
                    case azure::Key::d:
                        _d_pressed = false;
                        break;
					case azure::Key::Home:
                    case azure::Key::e:
                        _e_pressed = false;
                        break;
					case azure::Key::End:
                    case azure::Key::x:
                        _x_pressed = false;
                        break;
                    case azure::Key::o:
                        _o_pressed = false;
                        break;
                    case azure::Key::p:
                        _p_pressed = false;
                        break;
                    case azure::Key::LShift:
                    case azure::Key::RShift:
                        _shift_pressed = false;
                        break;
                    case azure::Key::LCtrl:
                    case azure::Key::RCtrl:
                        _ctrl_pressed = false;
                        break;
                    default:
                        break;
                }
            }
            else if (event->isType("mouseDown")) {
                if (event->getAttribute<azure::MouseButton>("button") == azure::MouseButton::Left) {
                    _tracking_mouse = true;
                }
            }
            else if (event->isType("mouseUp")) {
                if (event->getAttribute<azure::MouseButton>("button") == azure::MouseButton::Left) {
                    _tracking_mouse = false;
                }
            }
            else if (event->isType("mouseScroll")) {
                //int s = event->getAttribute<int>("y"); // magic number
                //_gui->mouseScroll(s);
            }
            else if (event->isType("mouseMove")) {
				if (_mouse_active) {
					int m_x = event->getAttribute<int>("x");
					int m_y = event->getAttribute<int>("y");
					if (_tracking_mouse == true) {
						int diff_x = m_x - _m_x;
						int diff_y = m_y - _m_y;
						float f_diff_x = diff_x;// *0.1125f;
						float f_diff_y = diff_y;// *0.1125f;

						_mouse_diff_x += f_diff_x;
						_mouse_diff_y += f_diff_y;

						//_camera->add_theta(f_diff_x);
						//_camera->add_phi(-f_diff_y);
					}
					_m_x = m_x;
					_m_y = m_y;
				}
            }
            else if (event->isType("windowEntered")) {
                _tracking_mouse = false;
            }
            else if (event->isType("windowLeft")) {
                _tracking_mouse = false;
            }
            else if (event->isType("quit")) {
                //quit = true;
			} else if (event->isType("camera_move")) {

				glm::vec3 loc = event->getAttribute<glm::vec3>("location");
				float theta = event->getAttribute<float>("theta");
				float phi = event->getAttribute<float>("phi");
				float fov = event->getAttribute<float>("fov");
				float aspect = event->getAttribute<float>("aspect");
				_camera->move_camera(loc, theta, phi);
				_camera->set_fov(fov);
				_camera->set_aspect(aspect);

			} else if (event->isType("camera_additional_shift_matrix")) {

				glm::mat4 mat = event->getAttribute<glm::mat4>("matrix");
				float fov = event->getAttribute<float>("fov");
				glm::vec3 loc = event->getAttribute<glm::vec3>("location");
				_camera->build_additional_displacement(loc);
				_camera->set_fov(fov);
				_camera->build_additional_shift_matrix(mat);

			}
            
            
            return false;
        }
        
        void camera_event_handler::move_camera() {

			auto time = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(time - _previous_time);
			_previous_time = time;

			bool update = false;
			float mouseSensitivity = 0.001125f;

			float movementSpeed = 50.25f;

			if (_shift_pressed) {
				movementSpeed = movementSpeed / 20.0f;
				//kMouseSensitivity = kMouseSensitivity / 20.0f;
			}
            
            if (_ctrl_pressed) {
                movementSpeed = movementSpeed * 20.0f;
            }

			std::unique_lock<std::mutex> lock(_camera_mutex);

            if (_reset) {
                _reset = false;
                _camera->move_camera(glm::vec3(0.0, 0.0, 0.0), 0, 0);
                _camera->set_aspect(16.0/9.0);
                _camera->set_fov(75.0);
            }
            
            if (_w_pressed) {
                glm::vec3 dir = _camera->get_gaze();
                float dist = 0.5f;
                _camera->add_location(dir * dist * movementSpeed * (float)dt.count());
            }
            
            if (_s_pressed) {
                glm::vec3 dir = _camera->get_gaze();
                float dist = -0.5f;
                _camera->add_location(dir * dist * movementSpeed * (float)dt.count());
            }
            
            if (_a_pressed) {
                glm::vec3 dir = _camera->get_gaze();
                glm::vec3 tup = _camera->get_up();
                glm::vec3 right = glm::cross(dir, tup);
                float dist = -0.5f;
                _camera->add_location(right * dist * movementSpeed * (float)dt.count());
            }
            
            if (_d_pressed) {
                glm::vec3 dir = _camera->get_gaze();
                glm::vec3 tup = _camera->get_up();
                glm::vec3 right = glm::cross(dir, tup);
                float dist = 0.5f;
                _camera->add_location(right * dist * movementSpeed * (float)dt.count());
            }
            
            if (_e_pressed) {
                _camera->add_location(glm::vec3(0.0, 0.5f, 0.0) * movementSpeed * (float)dt.count());
            }
            
            if (_x_pressed) {
                _camera->add_location(glm::vec3(0.0, -0.5f, 0.0) * movementSpeed * (float)dt.count());
            }
            
            if (_p_pressed) {
                _camera->add_fov(-0.5f * movementSpeed * (float)dt.count());
            }
            
            if (_o_pressed) {
                _camera->add_fov(0.5f * movementSpeed * (float)dt.count());
            }

			if (std::abs(_mouse_diff_x) > 0.0f) {
				_camera->add_theta(_mouse_diff_x * 180.0f * mouseSensitivity / M_PI);
				_mouse_diff_x = 0.0f;
			}

			if (std::abs(_mouse_diff_y) > 0.0f) {
				_camera->add_phi(-_mouse_diff_y * 180.0f * mouseSensitivity / M_PI);
				_mouse_diff_y = 0.0f;
			}
            
        }
        
        void camera_event_handler::load_camera_path(const std::string camera_path, float speed_factor, bool radians_to_degrees, bool quit_at_end) {
            std::vector<camera_path_node> paths;
            bool success = read_camera_path_json(camera_path.c_str(), paths);
            if (success) {
                if (radians_to_degrees) {
                    for (auto& p : paths) {
                        p.camera.theta = glm::degrees(p.camera.theta);// - 180.0f;
                        p.camera.phi = glm::degrees(p.camera.phi);
                    }
                }
                _camera_path = std::make_shared<camera_path_manager>(paths, speed_factor);
                _quit_at_end = quit_at_end;
            }
            frame_tick();
        }

        
        void camera_event_handler::write_camera_path(const std::string camera_path) const {
            
            if (_saved_camera_positions.size() > 1) {
                write_camera_path_json(camera_path.c_str(), _saved_camera_positions);
            }
            
        }

		void camera_event_handler::end_camera_path() {
			if (_camera_path != nullptr) {
				_camera_path->finish();
			}
		}
    }
}
