#include <sstream>
#include "openglProcess.hpp"

namespace mush {
	openglProcess::openglProcess(unsigned int width, unsigned int height) : imageProcess(), _bg_colour({ 0.0f, 0.0f, 0.0f, 1.0f }) {
		_width = width;
		_height = height;
	}

	openglProcess::~openglProcess() {

	}

	void openglProcess::opengl_init(std::shared_ptr<mush::opencl> context, bool init_framebuffer) {
		queue = context->getQueue();
		_flip = context->getKernel("flip_vertical");
		_copy = context->getKernel("copyImage");
		_srgb = context->getKernel("decodeSRGB");

#if defined(_WIN32) 

		glGetError();

		glCtx = wglGetCurrentContext();
		glHDC2 = wglGetCurrentDC();

		glCtx4 = wglCreateContext(glHDC2);
		_glException();

		if (wglShareLists(glCtx, glCtx4)) {
			putLog("---- Shared GL Textures.");
		} else {
			putLog("---- Failed to share GL Textures.");
		}
		_glException();

		int test = wglMakeCurrent(glHDC2, glCtx4);

#elif defined(__linux__)
		glCtx = glXGetCurrentContext();
#elif defined(__APPLE__)
		cgl_context = CGLGetCurrentContext();
		auto pix_fmt = CGLGetPixelFormat(cgl_context);

		if (auto err = CGLCreateContext(pix_fmt, cgl_context, &cgl_context_new)) {
			putLog(CGLErrorString(err));
			throw std::runtime_error("---- Failed to share GL Textures.");
		} else {
			putLog("---- Shared GL Textures.");
		}
		_glException();
		CGLSetCurrentContext(cgl_context_new);
#endif


		//_imgui_drawer = std::make_shared<imguiDrawer>(_width, _height);
		//_imgui_event_handler = std::make_shared<imguiEventHandler>(_imgui_drawer);

		//_imgui_drawer->resize(_width, _height, _width, _height);

		//videoMushAddEventHandler(_imgui_event_handler);

		if (init_framebuffer) {
			framebuffer_init(context);
		}

#if defined(_WIN32)
		test = wglMakeCurrent(glHDC2, glCtx);
#elif defined(__APPLE__)
		CGLSetCurrentContext(cgl_context);
#endif
		

		//addItem(context->floatImage(_width, _height));
	}
    
    void openglProcess::create_cl_framebuffer(std::shared_ptr<mush::opencl> context, unsigned int width, unsigned int height, cl::ImageGL * &cl_gl, std::shared_ptr<azure::Framebuffer> &frame) {
        
        frame = std::make_shared<azure::Framebuffer>();
        frame->setTarget(_target);
        frame->createTexture(GL_RGBA32F, width, height, GL_RGBA, GL_FLOAT, NULL);
        frame->attach();
        frame->clear();
        
        //frame->bind();
        //frame->attach();
        
        frame->createDepthTexture();
        /*
        glEnable(GL_DEPTH_TEST);
        _glException();
        glDepthMask(GL_TRUE);
        _glException();
        glDepthFunc(GL_LEQUAL);
        _glException();
        */
		frame->bind();
        glClearColor(_bg_colour[0], _bg_colour[1], _bg_colour[2], _bg_colour[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        _glException();
        
        frame->unbind();
        
        GLuint texID = frame->getTexture();
        
        cl_gl = context->glImage(texID, CL_MEM_READ_WRITE, _target, false);

    }

	void openglProcess::framebuffer_init(std::shared_ptr<mush::opencl> context) {

		_frame = std::make_shared<azure::Framebuffer>();
		_frame->setTarget(_target);
		_frame->createTexture(GL_RGBA32F, _width, _height, GL_RGBA, GL_FLOAT, NULL);
		_frame->attach();
		_frame->clear();

		//_frame->bind();
		//_frame->attach();

		_frame->createDepthTexture();

		_frame->bind();

		glEnable(GL_DEPTH_TEST);
		_glException();
		glDepthMask(GL_TRUE);
		_glException();
		glDepthFunc(GL_LEQUAL);
		_glException();

		glClearColor(_bg_colour[0], _bg_colour[1], _bg_colour[2], _bg_colour[3]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		_glException();

		_frame->unbind();

		GLuint texID = _frame->getTexture();


		_frameCL = context->glImage(texID, CL_MEM_READ_WRITE, _target, false);

		std::vector<cl::Memory> glObjects;
		glObjects.push_back(*_frameCL);
		cl::Event event;

		_glException();


		glFinish();
		queue->enqueueAcquireGLObjects(&glObjects, NULL, &event);
		event.wait();
		glFinish();
		_glException();

	}

	void openglProcess::bind_second_context() {
#if defined(_WIN32)
		int test = wglMakeCurrent(glHDC2, glCtx4);
		auto word = GetLastError();

		if (!test) {
			std::stringstream strm2;
			strm2 << "Err: " << word;
			putLog(strm2.str());
		}
#elif defined(__APPLE__)
		auto err = CGLSetCurrentContext(cgl_context_new);
		if (err < 0) {
			throw std::runtime_error(CGLErrorString(err));
		}
#endif
	}

	void openglProcess::unbind_second_context() {
#if defined(_WIN32)
		int test = wglMakeCurrent(glHDC2, glCtx);
		auto word = GetLastError();

		if (!test) {
			std::stringstream strm2;
			strm2 << "Err: " << word;
			putLog(strm2.str());
		}
#elif defined(__APPLE__)
		auto err = CGLSetCurrentContext(cgl_context);
		if (err < 0) {
			throw std::runtime_error(CGLErrorString(err));
		}
#endif
	}

	void openglProcess::bind_gl(bool clear) {
		if (!_run_once) {
			thread_init();
		}


		cl::Event event4;
		std::vector<cl::Memory> glObjects;
		glObjects.push_back(*_frameCL);

		cl::Event event2;

		glFinish();

		queue->enqueueReleaseGLObjects(&glObjects, NULL, &event2);
		event2.wait();

		_frame->bind(false);
		if (clear) {
			glClearColor(_bg_colour[0], _bg_colour[1], _bg_colour[2], _bg_colour[3]);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			_glException();
		}
	}

	void openglProcess::unbind_gl() {


		cl::Event event4;
		std::vector<cl::Memory> glObjects;
		glObjects.push_back(*_frameCL);

		//_frame->unbind();

		glFinish();

		cl::Event event3;
		queue->enqueueAcquireGLObjects(&glObjects, NULL, &event3);
		event3.wait();
	}

	void openglProcess::process_gl(postprocess_kernel flip) {

		inLock();

		cl::Event event;
		cl::Kernel * kern = _flip;

		switch (flip) {
		case postprocess_kernel::flip:
			kern = _flip;
			break;
		case postprocess_kernel::copy:
			kern = _copy;
			break;
		case postprocess_kernel::srgb:
			kern = _srgb;
			break;
		}

		kern->setArg(0, *_frameCL);
		kern->setArg(1, _getImageMem(0));

		queue->enqueueNDRangeKernel(*kern, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
		event.wait();

		inUnlock();
	}

	void openglProcess::thread_init() {
		if (!_run_once) {
#if defined(_WIN32)
			int test = wglMakeCurrent(glHDC2, glCtx4);
			auto word = GetLastError();

			if (!test) {
				std::stringstream strm2;
				strm2 << "Err: " << word;
				putLog(strm2.str());
			}
#elif defined(__APPLE__)
			auto err = CGLSetCurrentContext(cgl_context_new);
			if (err < 0) {
				throw std::runtime_error(CGLErrorString(err));
			}
#endif

			_run_once = true;

		}
	}

	void openglProcess::release_gl_assets() {
		if (queue != nullptr) {
			if (_frameCL != nullptr) {
				std::vector<cl::Memory> glObjects;
				glObjects.push_back(*_frameCL);
				//        glObjects.push_back(*_frameDepthCL);
				cl::Event event2;
				queue->enqueueReleaseGLObjects(&glObjects, NULL, &event2);
				event2.wait();

				delete _frameCL;
				_frameCL = nullptr;
			}
		}

		_frame = nullptr;
		//_imgui_drawer = nullptr;

		//if (_imgui_event_handler != nullptr) {
		//	videoMushRemoveEventHandler(_imgui_event_handler);
		//}

		//_imgui_event_handler = nullptr;
		
#ifdef _WIN32
		int success = 1;
		if (glCtx4 != NULL) {
			success = wglDeleteContext(glCtx4);
			glCtx4 = NULL;
		}
#else
		bool success = CGLDestroyContext(cgl_context_new);
#endif
		if (!success) {
			_glException();
		}
	}
}
