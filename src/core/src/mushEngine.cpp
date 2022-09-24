

#include "mushEngine.hpp"

#include <scarlet/Display.hpp>
#include <scarlet/Filter.hpp>
#include <azure/log.hpp>

#include "mushWindow.hpp"
#include "mushMainScreen.hpp"
#include "mushSidebar.hpp"

namespace mush {
	namespace gui {

		decltype(engine::_platformInterface) engine::_platformInterface;

		engine::engine() : azure::Engine() {

		}

		engine::~engine() {

		}

		void engine::mush_init() {

			_target = std::make_shared<azure::Framebuffer>();
			_target->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			_target->setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			_target->setClearColor(glm::vec4(0.0, 0.0, 0.0, 1.0));
			_target->createTexture(GL_RGBA32F, 1, 1, GL_RGB, GL_FLOAT, 0);
			_target->attach();

			glEnable(GL_SCISSOR_TEST);
			_glException();

			// Disable framebuffer read clamping.
			glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
			_glException();

			scarlet::GUI::Init();

		}

		std::shared_ptr<azure::Window> engine::addWindow(azure::WindowHandle window, azure::ContextHandle context, std::shared_ptr<mush::gui::sidebar> sidebar, std::shared_ptr<mush::gui::main_screen> main_screen, std::shared_ptr<mush::gui::screen_section> sim2) {
			std::lock_guard<decltype(_mutex)> lock(_mutex);
			auto w = std::make_shared<mush::gui::window>(window, context, _target, sidebar, main_screen, sim2);

			_windows[window] = w;
			return w;
		}

		void engine::getFrame(std::shared_ptr<mush::gui::sidebar> sidebar, std::shared_ptr<mush::gui::main_screen> main_screen, int stereo_pass) {

			auto main_s = sidebar->get_current_main();

			auto size = main_s->get_size();
			/*if (main_s->get_stereo()) {
				size.x = size.x / 2;
			}*/
			auto targetSize = _target->getSize();

			if (!_target->isAttached() || targetSize.x != size.x || targetSize.y != size.y) { // I don't like this much either. But we need to wait for a frame before we know what size to make the framebuffer.
																							  //ASSERT(size.x > 0);
																							  //ASSERT(size.x > 0);
				_target->createTexture(GL_RGBA32F, size.x, size.y, GL_RGB, GL_FLOAT, nullptr);
				_target->attach();
				_target->clear();
			}

			_target->bind();

			glScissor(0, 0, size.x, size.y);
			_glException();
			glViewport(0, 0, size.x, size.y);
			_glException();

			main_screen->set_screen(main_s);
			main_screen->render(stereo_pass);

			_target->unbind();

		}

		void engine::registerFilter(std::shared_ptr<scarlet::Filter> filter) {
			auto p = std::pair<const std::string, std::shared_ptr<scarlet::Filter>>(filter->getName(), filter);
			_filters.insert(p);
			lInfo << filter->getName() << " Filter registered";
		};

		std::shared_ptr<scarlet::Filter> engine::getFilter(const std::string & name) const {
			decltype(_filters)::const_iterator i;
			if ((i = _filters.find(name)) == _filters.end()) {
				throw std::runtime_error("No Filter named: " + name);
			}
			return std::make_shared<scarlet::Filter>(*i->second);
		}

		void engine::registerDisplay(std::shared_ptr<scarlet::Display> display) {
			auto p = std::pair<const std::string, std::shared_ptr<scarlet::Display>>(display->getName(), display);
			_displays.insert(p);
			lInfo << display->getName() << " Display Mode registered";
		};

		std::shared_ptr<scarlet::Display> engine::getDisplay(const std::string & name) const {
			decltype(_displays)::const_iterator i;
			if ((i = _displays.find(name)) == _displays.end()) {
				throw std::runtime_error("No Display Mode named: " + name);
			}
			return std::shared_ptr<scarlet::Display>((*i->second).clone());
		}

	}
}