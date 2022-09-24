//
//  mushSidebar.hpp
//  mush-core
//
//  Created by Josh McNamee on 21/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#ifndef mushSidebar_hpp
#define mushSidebar_hpp

#include <azure/framebuffer.hpp>
#include <azure/eventable.hpp>
#include <azure/sprite.hpp>

#include "guiScreenSection.hpp"

namespace mush {
	namespace gui {
		class sidebar : public azure::Sprite {
		public:
			sidebar(unsigned int screen_rows, unsigned int screen_width, unsigned int screen_height);
			~sidebar();

			void init();

			void render() override { render(0); }
			void render(int stereo_pass);

			std::shared_ptr<mush::gui::screen_section> add_screen();

			std::shared_ptr<mush::gui::screen_section> get_current_main() const;
			std::shared_ptr<mush::gui::screen_section> get_current_sim2() const;

			std::shared_ptr<mush::gui::screen_section> get_screen(int i) const;

			size_t get_screens_count() const {
				return _screens.size();
			}

			void set_scroll(float scroll);

			void click(int y, bool shift);
			void toggle();

			void resize(unsigned int new_width, unsigned int new_height);

			void add_rocket_gui(int i, std::string pathToRML);
			void init_rocket_guis();

			int get_current_main_int_legacy() const {
				return _current_main;
			}

			unsigned int get_width() const {
				return _width;
			}

			void clear() {
				_screens.clear();
			}
		private:

			unsigned int _screen_rows;

			unsigned int _screen_width, _screen_height;

			unsigned int _width, _height;

			int _current_main = 0;
			int _current_sim2 = 0;

			int _toggle = 0;

			std::vector<std::shared_ptr<mush::gui::screen_section>> _screens;

			float _scroll = 0.0;

			int swapped = 0;
			int tabbed = 0;

			std::vector<std::pair<int, std::string>> _rocket_gui_list;

			std::shared_ptr<azure::Framebuffer> _frame = nullptr;

		};
	}
}

#endif /* mushSidebar_hpp */
