//
//  mushSidebar.cpp
//  mush-core
//
//  Created by Josh McNamee on 21/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#include <mushSidebar.hpp>
#include <memory>

namespace mush {
	namespace gui {
		sidebar::sidebar(unsigned int screen_rows, unsigned int screen_width, unsigned int screen_height) : azure::Sprite(nullptr, nullptr), _screen_rows(screen_rows), _screen_width(screen_width), _screen_height(screen_height) {
			_height = screen_height;
			_width = _screen_width / (_screen_rows + 1);


			_frame = std::make_shared<azure::Framebuffer>();

			_frame->setTarget(GL_TEXTURE_2D);

			_frame->setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			_frame->setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			_frame->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			_frame->setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			_frame->applyParams();

			_frame->createTexture(GL_RGBA, _width, _height, GL_RGBA, GL_FLOAT, NULL);
			_frame->attach();
			_frame->clear();

			_texture = _frame;

			_frame->bind();
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			_glException();
			glClear(GL_COLOR_BUFFER_BIT);
			_glException();

			_frame->unbind();

			const char * vert = "shaders/null.vert";
			const char * frag = "shaders/null.frag";

			_program = std::make_shared<azure::Program>(vert, frag);
			_program->link();

			_program->use();
			_program->uniform("tex", getSampler());
			_program->discard();

			_layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_FALSE));
			_layout.push_back(azure::GLObject::Layout("vertTexCoord", 2, GL_FLOAT, GL_FALSE));

			init();


		}

		sidebar::~sidebar() {

		}

		void sidebar::init() {

			auto f_width = (float)2.0f;
			auto f_height = (float)2.0f;

			auto f_back_width = 0.0f;
			auto f_back_height = 0.0f;

			float x = 0.0f;
			float y = 0.0f;

			float w_ext = -1.0f + 4.0f;// * _width / (float)_screen_width;
			float h_ext = -1.0f + 4.0f;// * _height / (float)_screen_height;

			_data = {
				x + 1.0f,            y - 1.0f,            0.0f, f_back_width, f_back_height,
				x + 1.0f,            y + 1.0f * h_ext,    0.0f, f_back_width, f_height,
				x - 1.0f * w_ext,    y - 1.0f,		      0.0f, f_width, f_back_height/*,

				x - 1.0f,            y - 1.0f,            0.0f, f_width, f_back_height,
				x + 1.0f * w_ext,    y + 1.0f * h_ext,    0.0f, f_back_width, f_height,
				x + 1.0f * w_ext,    y - 1.0f,            0.0f, f_back_width, f_back_height*/
			};

			_count = 3;

			buffer();
			attach();
		}

		void sidebar::render(int stereo_pass) {
			if (_screens.size() > 1) {
				set_scroll(0);
				_frame->bind();
				//_frame->clear();



				glClearColor(0.5f, 0.5f, 0.0f, 1.0f);
				_glException();
				glClear(GL_COLOR_BUFFER_BIT);
				_glException();
				glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

				/*
				glScissor(0, 0, _width, _height);
				_glException();
				glViewport(0, 0, _width, _height);
				_glException();
				*/

				auto scissor_height = _height;
				if (_screens.size() < _screen_rows) { // +1 is fine b/c the main screen doesn't count
					scissor_height = _height - (_screens.size() - 1) * _height / _screen_rows;
				}


				for (int i = 0; i < _screens.size(); ++i) {
					if (i != _current_main) {
                        
						_screens[i]->render(_width, scissor_height, _height, stereo_pass);
					}
				}
				_frame->unbind();

				
				glScissor(0, 0, _width, _height);
				_glException();
				glViewport(0, 0, _width, _height);
				_glException();

				azure::Sprite::render();

				glScissor(0, 0, _screen_width, _screen_height);
				_glException();
				glViewport(0, 0, _screen_width, _screen_height);
				_glException();
				glDisable(GL_SCISSOR_TEST);
			}
		}

		std::shared_ptr<mush::gui::screen_section> sidebar::add_screen() {
			_screens.push_back(std::make_shared<mush::gui::screen_section>(1, _screen_width, _screen_height, _screen_rows));

			return _screens.back();
		}

		std::shared_ptr<mush::gui::screen_section> sidebar::get_current_main() const {
			return _screens[_current_main];
		}

		std::shared_ptr<mush::gui::screen_section> sidebar::get_current_sim2() const {
			return _screens[_current_sim2];
		}

		std::shared_ptr<mush::gui::screen_section> sidebar::get_screen(int i) const {
			return _screens[i];
		}

		void sidebar::set_scroll(float scroll) {
			_scroll += scroll;
			{
				float min = 2.0f*((float)(_screens.size() - 1) - (float)_screen_rows) / (float)_screen_rows;
				//float min = ((float)subSectionRows/2.0f)*((float)(subScreens.size()-1)-(float)subSectionRows)/(float)subSectionRows;
				if (_screens.size() > 0) {
					auto r = _screens[0]->get_ratio();
					auto diff = _height / (_height / (2.0f / r));
					_scroll = std::min(_scroll, r * (_screens.size() - 1) - diff*r);
				}

				//_scroll = std::min(_scroll, min);
				_scroll = std::max(_scroll, 0.0f);
			}

			int screens_added = 0;
			for (int i = 0; i < _screens.size(); ++i) {
				float scroll_base = _screens[i]->get_ratio();
				if (i != _current_main) {
					_screens[i]->setScroll(_scroll - scroll_base * screens_added);
					screens_added++;
				}
			}
		}

		void sidebar::click(int y, bool shift) {
			int in_height = ((_width / 16.0f) * 9.0f);

			int screens_counted = 1;
			for (int i = 0; i < _screens.size(); ++i) {
				if (i != _current_main) {
					if (y < ((screens_counted)*in_height) - (_scroll*((float)_screen_rows / 2.0f))*in_height) {
						if (shift) {
							_current_sim2 = i;
						} else {
							_toggle = _current_main;
							_current_main = i;
						}
						break;
					}
					screens_counted++;
				}
			}
		}

		void sidebar::toggle() {
			auto i = _current_main;
			_current_main = _toggle;
			_toggle = i;
		}

		void sidebar::resize(unsigned int new_width, unsigned int new_height) {
			_height = new_height;
			_width = new_width / (_screen_rows + 1);

			_screen_width = new_width;
			_screen_height = new_height;

			_frame->createTexture(GL_RGBA, _width, _height, GL_RGBA, GL_FLOAT, NULL);
			init();

			for (auto s : _screens) {
				s->resize(new_width, new_height);
			}
		}

		void sidebar::add_rocket_gui(int i, std::string pathToRML) {
			_rocket_gui_list.push_back(std::make_pair(i, pathToRML));
		}

		void sidebar::init_rocket_guis() {

			for (auto p : _rocket_gui_list) {
				if (p.first < _screens.size()) {
					_screens[p.first]->addGui(p.second.c_str(), p.first);
				}
			}

		}

	}
}
