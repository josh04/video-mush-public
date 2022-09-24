#ifndef FAKEWINDOW_HPP
#define FAKEWINDOW_HPP

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include "mushWindowInterface.hpp"

#include "node_api.hpp"

#ifdef MUSH_DX
#include "dxContext.hpp"
#else
typedef void* HWND;
#endif

#include "mush-core-dll.hpp"



namespace mush {
	namespace gui {
		class MUSHEXPORTS_API fake_window {
		public:
			fake_window(HWND hwnd = NULL);

			~fake_window();

			azure::ContextHandle get_context() const {
				return _context;
			}

			void acquire_context();

			void release_context();
	
		private:
			static std::atomic_int _count;
            static std::atomic_bool _created;
#ifdef MUSH_DX
			static std::atomic_bool _dx_created;
			static std::shared_ptr<mush::dx> _dx;
#endif
			static azure::WindowHandle _window;
			static azure::ContextHandle _context;

			static void * _hdc;
			
			static std::thread::id _current_thread_id;
			static std::condition_variable _cond;
			static std::mutex _thread_mutex;
			static bool _is_taken;
			
		};
	}
}

#endif
