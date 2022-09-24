#include <azure/eventtypes.hpp>
#include <azure/events.hpp>

#include <azure/engine.hpp>
#include <azure/log.hpp>
#include "mushWindow.hpp"
#include <scarlet/region.hpp>

#include <scarlet/display.hpp>
#include <scarlet/filter.hpp>

#include "guiScreenSection.hpp"
#include "mushSidebar.hpp"
#include "mushMainScreen.hpp"

namespace mush {
	namespace gui {

		window::window(azure::WindowHandle window,
			azure::ContextHandle context,
			std::shared_ptr<azure::Texture> texture,
			std::shared_ptr<mush::gui::sidebar> sidebar_,
			std::shared_ptr<mush::gui::main_screen> main_screen,
			std::shared_ptr<mush::gui::screen_section> sim2)
			: azure::Window(window, context), sim2(sim2), _sidebar(sidebar_), _main_screen(main_screen) {

			_target = std::make_shared<azure::Framebuffer>();
			_target->setTarget(GL_TEXTURE_RECTANGLE);

			_target->createTexture(GL_RGBA32F, 1, 1, GL_RGB, GL_FLOAT, 0);
			_target->attach();
			_target->clear();

			_gui = std::make_shared<scarlet::GUI>(window, context);

			addEventHandler(_gui);
			addUpdateHandler(_gui);

			_document = _gui->addFile(_rml);
			_document->AddReference();

			//scarlet::GUI::PythonEvent(_document, std::shared_ptr<azure::Event>(new azure::Event("init", 0)));

			_frame = std::make_shared<scarlet::Frame>(texture, _gui);
			_frame->init();

			addEventHandler(_frame);

			setDisplay(std::make_shared<scarlet::Display>("shaders/ldr.display")); // FIXME: Dubious
			auto e = std::shared_ptr<azure::Event>(new azure::Event("setDisplay", window));
			e->setAttribute<std::string>("name", "LDR");
			doEvent(e);

			sidebar = _document->GetElementById("sidebar");
			label = _document->GetElementById("label");
			fps = _document->GetElementById("fps");
			pause = _document->GetElementById("play");
			mode = _document->GetElementById("mode");
			lock = _document->GetElementById("lock");

		}

		window::~window() {

		}

		void window::doRender(bool wide, int stereo_pass) {

			auto windowInterface = azure::Engine::GetWindowInterface();
			windowInterface->makeCurrent(_window, _context);

			//glClear(GL_COLOR_BUFFER_BIT);

			_target->bind();
			//glClear(GL_COLOR_BUFFER_BIT);
			_glException();

			glScissor(0, 0, _width, _height);
			_glException();
			glViewport(0, 0, _width, _height);
			_glException();


			if (wide) {
				_frame->setWindowOffset({ _sidebar->get_width(), 0.0f });
				_frame->setWindowSize({ _width - _sidebar->get_width(), _height });
			} else {
				_frame->setWindowOffset({ 0.0f, 0.0f });
				_frame->setWindowSize({ _width, _height });
			}



			if (_sidebar->get_current_main_int_legacy() != _previous_main || (int)_sidebar->get_current_main()->getSelector() != _previous_selector) {
				_frame->recalculateAspectRatio();
				_previous_main = _sidebar->get_current_main_int_legacy();
				_previous_selector = (int)_sidebar->get_current_main()->getSelector();
			}

			_frame->render();


			if (wide) {
                pause->SetProperty("left", "11.5%");
				if (_sidebar != nullptr) {
					_sidebar->render(stereo_pass);
				}
            } else {
                pause->SetProperty("left", "0.5%");
            }

			if (sim2 != nullptr) {
				sim2->setTexture(_sidebar->get_current_sim2()->getTexture());
				sim2->getProgram()->use();
				sim2->render(1920, 0, 1080, 0);
				sim2->getProgram()->discard();

				//_width = 1920;
				//_height = 1080;
			}

			if (_fps_counter == 25) {
				azure::TimePoint newTime = azure::Clock::now();
				azure::Duration delta = newTime - _timeOfLastRender;
				_timeOfLastRender = newTime;
				float fps = 25.0f / (float)(delta.count());
				std::stringstream strm;
				strm << fps;
				setFPS(strm.str().c_str());
				_fps_counter = 0;

				if (_inner_fps != nullptr) {
					std::stringstream f_strm;
					f_strm << *_inner_fps;
					pause->SetInnerRML(f_strm.str().c_str());
				}
			}

			++_fps_counter;

			_gui->render(); // Render GUI on top.
			_target->unbind();

			// Finally render self to the screen with the correct display shader.
			//int w, h;
			//SDL_GL_GetDrawableSize((SDL_Window*)_window, &w, &h);
			glScissor(0, 0, _width, _height);
			_glException();
			glViewport(0, 0, _width, _height);
			_glException();
			//_program->use();
			//_program->uniform("tex", getSampler());
			//Sprite::render();
			//_program->discard();
			//_glException();

			_output->render();

			if (stereo_pass == 0 || stereo_pass == 2) {
				azure::Engine::GetWindowInterface()->swapWindow(_window);
			}
		}

		void window::setLabel(const char * name) {
			label->SetInnerRML(name);
		}

		void window::setFPS(const char * name) {
			//std::cout << name << std::endl;
			fps->SetInnerRML(name);
		}

		void window::setInnerFPS(float * innerfps) {
			_inner_fps = innerfps;
		}

		void window::setMode(const char * name) {
			mode->SetInnerRML(name);
		}

		void window::setLock(bool l) {
			if (l) {
				lock->SetAttribute("style", "visibility: visible;");
			} else {
				lock->SetAttribute("style", "visibility: hidden;");
			}
		}


		bool window::doEvent(std::shared_ptr<azure::Event> event) {
			if (event->getWindow() != _window) {
				return false;
			}

			if (Window::doEvent(event)) { return true; }

			// Reallocate the framebuffer texture and reattach.
			if (event->isType("windowResize")) {
				if (_width < 1 || _height < 1) { return false; }
				lInfo << "Window Resizing Width: " << _width << " Height: " << _height;
				_target->createTexture(GL_RGBA32F, _width, _height, GL_RGB, GL_FLOAT, 0);
				_target->attach();

				// Fix up data :/
				/*_data[8] = width;
				_data[18] = width;
				_data[28] = width;
				_data[14] = height;
				_data[24] = height;
				_data[29] = height;
				buffer();

				_gui->setViewport(uvec4(0, 0, width, height));*/ // The GUI could look after it's own viewport management.

				return true;
			}

			if (scarlet::GUI::PythonEvent(_document, event)) {
				return true;
			}

#define INTERNAL_CREATE_REGIONS
#if defined(INTERNAL_CREATE_REGIONS)

			if (event->isType("keyDown")) {
				switch (event->getAttribute<azure::Key>("key")) {
				case azure::Key::LCtrl:
					_shift = true;
					break;
				}
			}

			if (event->isType("keyUp")) {
				switch (event->getAttribute<azure::Key>("key")) {
				case azure::Key::LCtrl:
					_shift = false;
					_dragging = false;
					break;
				}
			}

#define SHIFT_REGIONS
#ifndef SHIFT_REGIONS
			_shift = true;
#endif
			if (event->isType("mouseDown") && _shift && event->getAttribute<azure::MouseButton>("button") == azure::MouseButton::Right) {
				_dragging = true;
				_stored_start = _start;
				_end = _stored_start;
				lInfo << "Dragging begin at: " << _stored_start.x << "," << _stored_start.y;
			} else if (event->isType("mouseUp") && _dragging && _shift && event->getAttribute<azure::MouseButton>("button") == azure::MouseButton::Right) {
				_dragging = false;
				glm::uvec2 s = glm::min(_stored_start, _end);
				glm::uvec2 e = glm::max(_stored_start, _end);
				glm::uvec4 region = glm::uvec4(
					s.x,
					s.y,
					e.x - s.x,
					e.y - s.y
				);
				lInfo << "Dragging end: " << _start.x << "," << _start.y;
				if (region.x >= 0 && region.y >= 0 && region.z >= 128 && region.w >= 128) { // Prevent tiny regions
					auto r = _frame->addRegion(region);

					// This is all pretty bad.
					static int zIndex = 0;
					++zIndex;
					Rocket::Core::String str;
					str.FormatString(127, "%d", zIndex);
					r->getFilter()->getDocument()->SetProperty("z-index", str);
				}
				_start = _end;
			} else if (event->isType("mouseUp") && _dragging) {
				_dragging = false;
			} else if (event->isType("mouseMove")) {
				if (_dragging) {
					_end.x = event->getAttribute<int>("x");
					_end.y = event->getAttribute<int>("y");
				} else {
					_start.x = event->getAttribute<int>("x");
					_start.y = event->getAttribute<int>("y");
				}
			}
#endif

			return false;
		}

		void window::setDisplay(std::shared_ptr<scarlet::Display> display) {
			if (display == nullptr) { return; }

			if (_output != nullptr) { removeEventHandler(_output); }
			//_output = shared_ptr<Display>(display->clone());
			_output = display;
			_output->setTexture(_target);
			addEventHandler(_output);

			_output->event(std::make_shared<azure::WindowResizeEvent>(nullptr, _width, _height));
		}

		void window::raiseEvent(std::shared_ptr<azure::Event> event) {
			event->setWindow(_window);
			azure::Events::Push(std::unique_ptr<azure::Event>(new azure::Event(*event)));
		}
	}
}
