//
//  sphereMapProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 05/12/2016.
//
//

#include "sphereMapProcess.hpp"

//#include "exports.hpp"
#ifdef __APPLE__
#include <Cocoa/Cocoa.h>

//#include <CoreFoundation/CoreFoundation.h>

#include <azure/glm/glm.hpp>
#include <azure/glm/gtc/matrix_transform.hpp>
#include <azure/glm/gtx/transform.hpp>
#else
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#endif
#include <azure/eventtypes.hpp>

namespace mush {
    sphereMapProcess::sphereMapProcess(unsigned int width, unsigned int height) : mush::openglProcess(width, height) {
        
        _camera = std::make_shared<mush::camera::base>();
        _camera_event_handler = std::make_shared<mush::camera::camera_event_handler>(_camera);
        
    }
    
    sphereMapProcess::sphereMapProcess(unsigned int width, unsigned int height, bool auto_cam, const char * auto_cam_path, float auto_cam_speed, bool quit_at_end) : mush::openglProcess(width, height), auto_cam(auto_cam), auto_cam_path(auto_cam_path), auto_cam_speed(auto_cam_speed), _quit_at_end(quit_at_end) {
        
        _camera = std::make_shared<mush::camera::base>();
        _camera_event_handler = std::make_shared<mush::camera::camera_event_handler>(_camera);
        
        if (auto_cam) {
            _camera_event_handler->load_camera_path(auto_cam_path, auto_cam_speed, false, quit_at_end);
        }
        
    }
    
    void sphereMapProcess::release() {
		if (queue != nullptr) {
			if (_input_texture != nullptr) {
				std::vector<cl::Memory> glObjects;
				glObjects.push_back(*_input_texture);
				//        glObjects.push_back(*_frameDepthCL);
				cl::Event event2;
				//queue->enqueueReleaseGLObjects(&glObjects, NULL, &event2);
				//event2.wait();

				delete _input_texture;
				_input_texture = nullptr;
			}
		}
		_texture = nullptr;

        //videoMushRemoveEventHandler(_camera_event_handler);
		_sphere = nullptr;
		_camera_event_handler = nullptr;
		_program->discard();
		_program = nullptr;
        release_local_gl_assets();
        imageProcess::release();
    }
    
    sphereMapProcess::~sphereMapProcess() {
    }
    
    void sphereMapProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {

		opengl_init(context);
		bind_second_context();
        assert(buffers.size() == 0 || buffers.size() == 1);
        
        _texture = std::make_shared<azure::Texture>();
        
        if (buffers.size() == 1 && buffers.begin()[0] != nullptr) {
            _input = castToImage(buffers.begin()[0]);
            
            unsigned int sz;
            _input->getParams(tex_width, tex_height, sz);
            
            _texture->createTexture(GL_RGBA32F, tex_width, tex_height, GL_RGBA, GL_FLOAT, NULL);
            
            
            _texture->setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            _texture->setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            //_texture->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            _texture->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            //_texture->setParam(GL_GENERATE_MIPMAP, GL_TRUE);
            _texture->setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            _texture->applyParams();
            
            _input_texture = context->glImage(_texture->getTexture(), CL_MEM_READ_WRITE, _texture->getTarget(), false);
            
        }
        
		const char * vert = "shaders/sphere.vert";
		const char * frag = "shaders/sphere.frag";
/*
#ifdef __APPLE__
		//CFBundleRef requestedBundle = CFBundleGetBundleWithIdentifier(CFSTR("josh04.parcore-framework") );

		NSString * frDir = [NSString stringWithFormat : @"%@/%@",[[NSBundle mainBundle] bundlePath], @"Contents/Frameworks/Video Mush.framework"];
			NSString * resDir = [[[NSBundle bundleWithPath : frDir] resourcePath] stringByAppendingString:@"/"];
			NSString * NSvertStr = [resDir stringByAppendingString : [NSString stringWithUTF8String : vert]];
		NSString * NSfragStr = [resDir stringByAppendingString : [NSString stringWithUTF8String : frag]];
		vert = [NSvertStr UTF8String];
		frag = [NSfragStr UTF8String];
#endif
*/
		_program = std::make_shared<azure::Program>(vert, frag);
		_program->use();
		_program->uniform("gammma", 1.0f);

        _sphere = std::make_shared<sphereGL>(_program, glm::vec3(0.0f, 0.0f, 0.0f), 100.0f);
        
        _bg_colour = {0.18f, 0.18f, 0.18f, 1.0f};
        
		_camera->set_fov(75.0f);
		_camera->set_aspect(16.0f/9.0f);


        //videoMushAddEventHandler(_camera_event_handler);
        
        _program->discard();
        
		unbind_second_context();

		addItem(context->floatImage(_width, _height));
        
        _input_token = _input->takeFrameToken();
    }
    
    void sphereMapProcess::process() {
        _tick++;
        bind_gl();

        if (_input.get() != nullptr) {
        
            cl::Event event3;
            std::vector<cl::Memory> glObjects;
            glObjects.push_back(*_input_texture);

            queue->enqueueAcquireGLObjects(&glObjects, NULL, &event3);
            event3.wait();

            //auto ptr = _input->outLock(16000000);
            //auto ptr = _input->outLock(16000000, _input_token);
			auto ptr = _input->outLock(0, _input_token);
            if (ptr == nullptr) {
                //_input = nullptr;
            } else {
                /*if (auto_cam && !_camera_event_handler->camera_path_finished()) {
                    _camera_event_handler->frame_tick();
                    _camera->move_camera({0,0,0}, 90.0f, _camera->get_phi());
                    _camera->set_fov(58.7155);
                }*/
                
                _flip->setArg(0, ptr.get_image());
                _flip->setArg(1, *_input_texture);
            
                cl::Event event;
                queue->enqueueNDRangeKernel(*_flip, cl::NullRange, cl::NDRange(tex_width, tex_height), cl::NullRange, NULL, &event);
                event.wait();
            
                _input->outUnlock();
            }
            cl::Event event2;
            queue->enqueueReleaseGLObjects(&glObjects, NULL, &event2);
            event2.wait();
            
            if (ptr == nullptr && _quit_at_end) {
                azure::Events::Push(std::unique_ptr<azure::Event>(new azure::QuitEvent()));
                /*unbind_gl();
                return;
				auto in_l = inLock();
				in_l = buffer();
				inUnlock();
                return;*/
            }
        }

        glViewport(0, 0, _width, _height);
        _glException();
        
        _program->use();
        _program->uniformV("M", false, &(_camera->get_model())[0][0]);
        _program->uniformV("V", false, &(_camera->get_view())[0][0]);
        //    _program->uniformV("MV", false, &MV[0][0]);
        _program->uniformV("MVP", false, &(_camera->get_mvp())[0][0]);
        
        
        _program->uniform("gamma", 1.0f);
        
        _program->uniform("tex", 0);
        
        auto from = _camera->get_location();
        
        GLfloat position[] = { from[0], from[1], from[2], 1.0f };
        
        GLint location;
        if ((location = _program->uniform("light_position")) < 0) {
            return;
        }
        glUniform3fv(location, 1, position);
        _glException();

        
        glEnable(GL_CULL_FACE);
        _glException();
        glCullFace(GL_BACK);
        _glException();
        
        glActiveTexture(GL_TEXTURE0);
        _glException();
        _texture->bind();
        _sphere->render();
        _texture->unbind();
        
        
        unbind_gl();

		auto buf = inLock();
		auto loc = _camera->get_location();
		buf.set_camera_position({loc.x, loc.y, loc.z}, { _camera->get_theta(), _camera->get_phi(), _camera->get_fov() });

		cl::Event event;
		cl::Kernel * kern = _flip;

		kern->setArg(0, *_frameCL);
		kern->setArg(1, _getImageMem(0));

		queue->enqueueNDRangeKernel(*kern, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
		event.wait();

		inUnlock();



		frame_tick();
		//_camera_event_handler->frame_tick();
		_camera_event_handler->move_camera();

    }
    
    
    void sphereMapProcess::release_local_gl_assets() {
        
        release_gl_assets();
    }

	void sphereMapProcess::frame_tick() {
		if (auto_cam) {
			_camera_event_handler->frame_tick();

				_camera->move_camera({ 0,0,0 }, 0, _camera->get_phi());
				//_camera->set_fov(58.7155f);
		}
	}
    
}
