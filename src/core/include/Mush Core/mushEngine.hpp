//
//  mushEngine.hpp
//  video-mush
//
//  Created by Josh McNamee on 18/03/2015.
//
//

#ifndef video_mush_mushEngine_hpp
#define video_mush_mushEngine_hpp

#include <azure/engine.hpp>

namespace azure {
	class Framebuffer;
}

namespace scarlet {
	class Filter;
	class Display;
	class PlatformInterface;
}

namespace mush {
	namespace gui {
		class window;
		class main_screen;
		class sidebar;
		class screen_section;

		class engine : public azure::Engine, public std::enable_shared_from_this<engine> {
		public:
			engine();

			~engine();

			void mush_init();

			std::shared_ptr<azure::Window> addWindow(azure::WindowHandle window, azure::ContextHandle context, std::shared_ptr<mush::gui::sidebar> sidebar, std::shared_ptr<mush::gui::main_screen> main_screen, std::shared_ptr<mush::gui::screen_section> sim2 = nullptr);

			void getFrame(std::shared_ptr<mush::gui::sidebar> sidebar, std::shared_ptr<mush::gui::main_screen> main_screen, int stereo_pass);


			void registerFilter(std::shared_ptr<scarlet::Filter> filter);

			std::shared_ptr<scarlet::Filter> getFilter(const std::string & name) const;

			std::map<const std::string, std::shared_ptr<scarlet::Filter>> getFilters() const {
				return _filters;
			}

			void registerDisplay(std::shared_ptr<scarlet::Display> display);

			std::shared_ptr<scarlet::Display> getDisplay(const std::string & name) const;

			std::map<const std::string, std::shared_ptr<scarlet::Display>> getDisplays() const {
				return _displays;
			}

		protected:
			std::map<const std::string, std::shared_ptr<scarlet::Filter>> _filters;

			std::map<const std::string, std::shared_ptr<scarlet::Display>> _displays;

			std::shared_ptr<azure::Framebuffer> _target;

			static std::shared_ptr<scarlet::PlatformInterface> _platformInterface;

			void runTimer() override {}; // Stop these being called.
			void runEvent() override {};

			engine(const engine &) = delete;
			void operator=(const engine &) = delete;

			void pythonInit();

		};
	}
}
#endif
