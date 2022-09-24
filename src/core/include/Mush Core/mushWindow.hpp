//
//  mushWindow.hpp
//  video-mush
//
//  Created by Josh McNamee on 18/03/2015.
//
//

#ifndef video_mush_mushWindow_hpp
#define video_mush_mushWindow_hpp

#include <azure/window.hpp>
#include <azure/Sprite.hpp>
#include <azure/Renderable.hpp>
#include <azure/chrono.hpp>

#include <Rocket/Core.h>
#include <sstream>

#include <SDL2/SDL.h>

//#include "exports.hpp"
#include <scarlet/gui.hpp>
#include <scarlet/frame.hpp>

namespace scarlet {
	class Display;
	class Filter;
}

namespace mush {
	namespace gui {
		class main_screen;
		class sidebar;
		class screen_section;

		class window : public azure::Window {
		public:
			window(azure::WindowHandle window,
				azure::ContextHandle context,
				std::shared_ptr<azure::Texture> texture,
				std::shared_ptr<mush::gui::sidebar> sidebar_,
				std::shared_ptr<mush::gui::main_screen> main_screen,
				std::shared_ptr<mush::gui::screen_section> sim2 = nullptr);

			~window();

			void doRender(bool wide, int stereo_pass);

			void setLabel(const char * name);

			void setFPS(const char * name);

			void setInnerFPS(float * innerfps);

			void setMode(const char * name);

			void setLock(bool l);



			bool doEvent(std::shared_ptr<azure::Event> event);
			void setDisplay(std::shared_ptr<scarlet::Display> display);
			void raiseEvent(std::shared_ptr<azure::Event> event);

		private:
			std::shared_ptr<mush::gui::main_screen> _main_screen = nullptr;
			std::shared_ptr<mush::gui::screen_section> sim2 = nullptr;

			Rocket::Core::Element * sidebar = nullptr, *label = nullptr, *fps = nullptr, *pause = nullptr, *mode = nullptr, *lock = nullptr;

			float * _inner_fps = nullptr;

			azure::TimePoint _timeOfLastRender;
			unsigned int _fps_counter = 0;

			std::shared_ptr<mush::gui::sidebar> _sidebar = nullptr;


			std::shared_ptr<azure::Framebuffer> _target = nullptr;
			std::shared_ptr<scarlet::Frame> _frame = nullptr;
			std::shared_ptr<scarlet::Display> _output;
			std::shared_ptr<scarlet::GUI> _gui = nullptr;

			Rocket::Core::ElementDocument * _document = nullptr;

#ifdef HAVE_PYTHON
			std::shared_ptr<boost::python::api::object> _scarletUpdate;
#endif

			bool _dragging = false;
			bool _shift = false;
			glm::uvec2 _start = glm::uvec2(0, 0);
			glm::uvec2 _stored_start = glm::uvec2(0, 0);
			glm::uvec2 _end = glm::uvec2(0, 0);

			std::string _rml = "player2.rml";

			int _previous_main = 0;
			int _previous_selector = 0;

		};
	}
}
#endif
