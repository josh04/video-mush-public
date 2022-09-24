#ifndef OPENGLPROCESS_HPP
#define OPENGLPROCESS_HPP

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#include <OpenGL/OpenGL.h>
#else
#include <GL/glew.h>
//#include <GL/gl.h>
#endif

#include <array>
#include <azure/framebuffer.hpp>

#include "opencl.hpp"

#include "mush-core-dll.hpp"
#include "imageProcess.hpp"
#include "openglProcess.hpp"

namespace mush {
	class MUSHEXPORTS_API openglProcess : public imageProcess {
	public:
		openglProcess(unsigned int width, unsigned int height);
		~openglProcess();

		void opengl_init(std::shared_ptr<mush::opencl> context, bool init_framebuffer = true);

		void framebuffer_init(std::shared_ptr<mush::opencl> context);

		void bind_second_context();
		void unbind_second_context();

		void bind_gl(bool clear = true);
		void unbind_gl();

		enum class postprocess_kernel {
			flip,
			copy,
			srgb
		};
		void process_gl(postprocess_kernel flip = postprocess_kernel::flip);

		void thread_init();
		void release_gl_assets();

		void set_target(GLenum target) {
			_target = target;
		}

		GLenum get_target() {
			return _target;
		}

		bool has_run_once() const {
			return _run_once;
		}
    protected:
        
        void create_cl_framebuffer(std::shared_ptr<mush::opencl> context, unsigned int width, unsigned int height, cl::ImageGL * &cl_gl, std::shared_ptr<azure::Framebuffer> &frame);
        
        cl::Kernel * _flip = nullptr;
		cl::Kernel * _copy = nullptr;
		cl::Kernel * _srgb = nullptr;
        cl::ImageGL * _frameCL = nullptr;
		std::shared_ptr<azure::Framebuffer> _frame = nullptr;
        std::array<float, 4> _bg_colour;
	private:

		//std::shared_ptr<imguiDrawer> _imgui_drawer = nullptr;
		//std::shared_ptr<imguiEventHandler> _imgui_event_handler = nullptr;


#ifdef _WIN32
		HGLRC glCtx;
		HGLRC glCtx4;
		HDC glHDC2;
#endif
#ifdef __APPLE__
		CGLContextObj cgl_context;
		CGLContextObj cgl_context_new;
#endif

		bool _run_once = false;

		GLenum _target = GL_TEXTURE_RECTANGLE;
	};
}

#endif
