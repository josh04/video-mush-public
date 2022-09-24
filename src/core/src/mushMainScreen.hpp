//
//  mushMainScreen.hpp
//  mush-core
//
//  Created by Josh McNamee on 22/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#ifndef mushMainScreen_hpp
#define mushMainScreen_hpp

#include <memory>

#include <azure/Sprite.hpp>

namespace mush {
	namespace gui {
		class dummy_eventable;
		class screen_section;

		class main_screen : public azure::Sprite {
		public:
			main_screen(unsigned int screen_rows);
			~main_screen();

			void set_vertex(bool wide);

			void set_screen(std::shared_ptr<mush::gui::screen_section> screen);
			void set_screen(std::shared_ptr<azure::Program> program, std::shared_ptr<azure::Texture> texture, bool stereo);

			void render() override { render(0); }
			void render(int stereo_pass);

			std::shared_ptr<mush::gui::dummy_eventable> get_eventable() const {
				return _dummy_event_holder;
			}
		private:
			void set_stereo_left();
			void set_stereo_right();
			void set_stereo_off();
			bool _stereo = false;
			unsigned int _screen_rows = 8;

			std::shared_ptr<mush::gui::dummy_eventable> _dummy_event_holder = nullptr;
		};
	}
}

#endif /* mushMainScreen_hpp */
