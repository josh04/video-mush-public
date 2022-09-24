#include "node_api.hpp"
#include "fakeWindow.hpp"
#include <azure/glexception.hpp>

namespace mush {
	namespace gui {

		std::atomic_int fake_window::_count;
		std::atomic_bool fake_window::_created; // check this for default
        
#ifdef MUSH_DX
		std::atomic_bool fake_window::_dx_created = false;
		std::shared_ptr<mush::dx> fake_window::_dx = nullptr;
#endif
		azure::WindowHandle fake_window::_window = nullptr;
		azure::ContextHandle fake_window::_context = nullptr;
		void * fake_window::_hdc = nullptr;


		std::thread::id fake_window::_current_thread_id;
		std::condition_variable fake_window::_cond;
		std::mutex fake_window::_thread_mutex;
		bool fake_window::_is_taken = false;

		fake_window::fake_window(HWND hwnd) {
			if (!_created.exchange(true)) {
				auto scarlet_interface = std::make_shared<mush::gui::window_interface>("");

				uint32_t flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
				_window = scarlet_interface->createWindow("", 1, 1, false, flags);
				_context = scarlet_interface->createContext(_window);

#ifdef _WIN32
				_hdc = wglGetCurrentDC();
#endif

				//_tg = make_shared<tagInGui>();
				//_tg->createGLContext(false, false, "");
                
#ifdef MUSH_DX
				if (hwnd != NULL) {
					_dx = std::make_shared<mush::dx>();
					_dx->init(hwnd);
				}
#endif

				//acquire_context();
			} else {
#ifdef _WIN32
				wglMakeCurrent((HDC)_hdc, (HGLRC)_context);
				_glException();
#endif
			}
			_count++;
		}

		fake_window::~fake_window() {
			
			if (_count.fetch_sub(1) == 1) {
				auto context = _context;
				auto window = _window;
				_created.exchange(false);
				auto scarlet_interface = std::make_shared<mush::gui::window_interface>("");
				scarlet_interface->destroyContext(context);
				scarlet_interface->destroyWindow(window);
                
#ifdef MUSH_DX
				_dx = nullptr;
				_dx_created.exchange(false);
#endif
			}
		}

		
		void fake_window::acquire_context() {
			std::unique_lock<std::mutex> lock(_thread_mutex);
			if (std::this_thread::get_id() != _current_thread_id) {
				_cond.wait(lock, [&]() { return _is_taken == false; });
			}
			_is_taken = true;
			_current_thread_id = std::this_thread::get_id();
            
#ifdef _WIN32
			wglMakeCurrent((HDC)_hdc, (HGLRC)_context);
			_glException();
#endif
		}

		void fake_window::release_context() {
			std::unique_lock<std::mutex> lock(_thread_mutex);
			if (std::this_thread::get_id() == _current_thread_id) {
				_is_taken = false;
				_cond.notify_all();
                
#ifdef _WIN32
				wglMakeCurrent(NULL, NULL);
#endif
			}
		}
		

	}
}
