//
//  mushMainScreen.cpp
//  mush-core
//
//  Created by Josh McNamee on 22/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#include "mushMainScreen.hpp"
#include "guiScreenSection.hpp"
#include "dummyEventable.hpp"

namespace mush {
	namespace gui {
		main_screen::main_screen(unsigned int screen_rows) : azure::Sprite(nullptr, nullptr), _screen_rows(screen_rows) {
			set_vertex(false);

			_dummy_event_holder = std::make_shared<mush::gui::dummy_eventable>();
		}

		main_screen::~main_screen() {

		}


		void main_screen::set_vertex(bool wide) {

			_mode = GL_TRIANGLES;
			_layout.clear();
			_layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_TRUE));
			_layout.push_back(azure::GLObject::Layout("vertTexCoord", 2, GL_FLOAT, GL_FALSE));

			_first = 0;
			_count = 3;

			float heightadd = 0.0f, widthadd = 2.0f / (float)(_screen_rows + 1);

			//if (!wide) {
			widthadd = 0.0f;
			//}

			_data = {
				-1.0f,						1.0f,               0.0f,	0.0f, 0.0f,
				-1.0f,						-3.0f,				 0.0f,	0.0f, 2.0f,
				3.0f,						1.0f,               0.0f,	2.0f, 0.0f,
				/*
				1.0f,                      1.0f,               0.0f,	1.0f, 0.0f,
				1.0f,                     -1.0f + (heightadd),	0.0f,	1.0f, 1.0f,
				-1.0f + widthadd,          -1.0f + (heightadd),	0.0f,	0.0f, 1.0f
				*/
			};

			buffer();
		}

		void main_screen::set_screen(std::shared_ptr<mush::gui::screen_section> screen) {
			set_screen(screen->getProgram(), screen->getTexture(), screen->get_stereo());
			_dummy_event_holder->set_eventables(screen->get_eventables());
		}

		void main_screen::set_screen(std::shared_ptr<azure::Program> program, std::shared_ptr<azure::Texture> texture, bool stereo) {
			_program = program;
			_texture = texture;
			_stereo = stereo;
			/*if (_stereo == false) {
				set_stereo_off();
			}*/
			attach();
		}

		void main_screen::render(int stereo_pass) {

			if (_stereo) {
				if (stereo_pass != 0) {
				if (stereo_pass == 2) {
					set_stereo_left();
				} else {
					set_stereo_right();
				}
				} else {
					set_stereo_off();
				}
			}

			_program->use();
			_program->uniform("scroll", 0.0f);
			azure::Sprite::render();
		}

		void main_screen::set_stereo_left() {
			_data[3] = 0.0f;
			_data[8] = 0.0f;
			_data[13] = 1.0f;
			/*
			_data[18] = 0.5f;
			_data[23] = 0.5f;
			_data[28] = 0.0f;
			*/
			buffer();
			attach();
		}

		void main_screen::set_stereo_right() {
			_data[3] = 1.0f;
			_data[8] = 1.0f;
			_data[13] = 2.0f;
			/*
			_data[18] = 1.0f;
			_data[23] = 1.0f;
			_data[28] = 0.5f;
			*/
			buffer();
			attach();
		}

		void main_screen::set_stereo_off() {
			_data[3] = 0.0f;
			_data[8] = 0.0f;
			_data[13] = 2.0f;
/*
			_data[18] = 1.0f;
			_data[23] = 1.0f;
			_data[28] = 0.0f;
			*/
			buffer();
			attach();
		}
	}
}