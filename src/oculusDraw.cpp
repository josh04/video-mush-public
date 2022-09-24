


#ifdef _WIN32
#include <windows.h>
#include <GL/glew.h>
#endif


//#define __glew_h__
//#define __GLEW_H__
#include <Mush Core/opencl.hpp>

#include "oculusDraw.hpp"
#include "exports.hpp"

#include "oculus/Win32_GLAppUtil.h"

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


#if defined(_WIN32)
#include <dxgi.h> // for GetDefaultAdapterLuid
#pragma comment(lib, "dxgi.lib")
#endif

static ovrGraphicsLuid GetDefaultAdapterLuid()
{
	ovrGraphicsLuid luid = ovrGraphicsLuid();

#if defined(_WIN32)
	IDXGIFactory* factory = nullptr;

	if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
	{
		IDXGIAdapter* adapter = nullptr;

		if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
		{
			DXGI_ADAPTER_DESC desc;

			adapter->GetDesc(&desc);
			memcpy(&luid, &desc.AdapterLuid, sizeof(luid));
			adapter->Release();
		}

		factory->Release();
	}
#endif

	return luid;
}

static int Compare(const ovrGraphicsLuid& lhs, const ovrGraphicsLuid& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}

namespace mush {
	oculusDraw::oculusDraw() : mush::openglProcess(1280, 720) {
		view_thread_over = false;
	}

	void oculusDraw::release() {

		view_thread_over = true;

		for (int eye = 0; eye < 2; ++eye)
		{
			delete eyeRenderTexture[eye];
			delete eyeDepthBuffer[eye];
		}


		if (queue != nullptr) {
			if (_eye_left != nullptr) {
				/*
				std::vector<cl::Memory> glObjects;
				glObjects.push_back(*_eye_left);
				glObjects.push_back(*_eye_right);
				//        glObjects.push_back(*_frameDepthCL);
				cl::Event event2;
				queue->enqueueReleaseGLObjects(&glObjects, NULL, &event2);
				event2.wait();
				*/
				delete _eye_left;
				delete _eye_right;
				_eye_left = nullptr;
				_eye_right = nullptr;
			}
		}

		//videoMushRemoveEventHandler(_camera_event_handler);
		//_sphere = nullptr;
		_left_eye_sprite = nullptr;
		_right_eye_sprite = nullptr;
//		_camera_event_handler = nullptr;
		_program->discard();
		_program = nullptr;
		release_local_gl_assets();

		if (mirrorFBO) glDeleteFramebuffers(1, &mirrorFBO);
		if (mirrorTexture) {
			ovr_DestroyMirrorTexture(session, mirrorTexture);
		}

		ovr_Destroy(session);

		imageProcess::release();
	}

	oculusDraw::~oculusDraw() {
	}

	void oculusDraw::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {

		opengl_init(context, false);
		//bind_second_context();
		assert(buffers.size() == 1 || buffers.size() == 2);

		auto l_tex = std::make_shared<azure::Texture>();
		auto r_tex = std::make_shared<azure::Texture>();

		//if (buffers.size() == 1 && buffers.begin()[0] != nullptr) {
			_input_left = castToImage(buffers.begin()[0]);
			if (buffers.size() > 1) {
				_input_right = castToImage(buffers.begin()[1]);
			}

			unsigned int sz;
			_input_left->getParams(tex_width, tex_height, sz);

			l_tex->createTexture(GL_RGBA32F, tex_width, tex_height, GL_RGBA, GL_FLOAT, NULL);
			r_tex->createTexture(GL_RGBA32F, tex_width, tex_height, GL_RGBA, GL_FLOAT, NULL);

			l_tex->setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			l_tex->setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			l_tex->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			l_tex->setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			l_tex->applyParams();
			r_tex->setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			r_tex->setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			r_tex->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			r_tex->setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			r_tex->applyParams();

			_eye_left = context->glImage(l_tex->getTexture(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, false);
			_eye_right = context->glImage(r_tex->getTexture(), CL_MEM_READ_WRITE, GL_TEXTURE_2D, false);

		//}

		const char * vert = "shaders/null_with_gamma.vert";
		const char * frag = "shaders/null_with_gamma.frag";

#ifdef __APPLE__
		//CFBundleRef requestedBundle = CFBundleGetBundleWithIdentifier(CFSTR("josh04.parcore-framework") );

		NSString * frDir = [NSString stringWithFormat : @"%@/%@",[[NSBundle mainBundle] bundlePath], @"Contents/Frameworks / Video Mush.framework"];
			NSString * resDir = [[[NSBundle bundleWithPath : frDir] resourcePath] stringByAppendingString:@"/"];
			NSString * NSvertStr = [resDir stringByAppendingString : [NSString stringWithUTF8String : vert]];
		NSString * NSfragStr = [resDir stringByAppendingString : [NSString stringWithUTF8String : frag]];
		vert = [NSvertStr UTF8String];
		frag = [NSfragStr UTF8String];
#endif

		_program = std::make_shared<azure::Program>(vert, frag);

		bind_second_context();
		_left_eye_sprite = std::make_shared<oculusEyeSprite>(_program, l_tex);
		_right_eye_sprite = std::make_shared<oculusEyeSprite>(_program, r_tex);
		//_sphere = std::make_shared<sphereGL>(_program, glm::vec3(0.0f, 0.0f, 0.0f), 10.0f);
		unbind_second_context();

		_bg_colour = { 0.18f, 0.18f, 0.18f, 1.0f };

		//_camera = std::make_shared<mush::camera::base>();
		//_camera_event_handler = std::make_shared<mush::camera::camera_event_handler>(_camera);
		//videoMushAddEventHandler(_camera_event_handler);


		//
		//
		//
		//
		//
		//
		// OCULUS GO

		ovrResult result2 = ovr_Initialize(nullptr);
		if (!OVR_SUCCESS(result2)) {
			throw std::runtime_error("Oculus: Failed to initialize libOVR.");
		}

		bool retryCreate = false;
		ovrResult session_create;
		for (int i = 0; i < 5; ++i) {
			session_create = ovr_Create(&session, &luid);
			if (OVR_SUCCESS(session_create)) {
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		if (!OVR_SUCCESS(session_create)) {
			throw std::runtime_error("No HMD detected.");
		}

		if (Compare(luid, GetDefaultAdapterLuid())) // If luid that the Rift is on is not the default adapter LUID...
		{
			throw std::runtime_error("Oculus: OpenGL supports only the default graphics adapter.");
		}

		/*ovrHmdDesc */hmdDesc = ovr_GetHmdDesc(session);

		
		/*
		if (!Platform.InitDevice(windowSize.w, windowSize.h, reinterpret_cast<LUID*>(&luid))) {
			//goto Done;
			throw std::runtime_error("Couldn't initialise device.");
		}
		*/

		_width = hmdDesc.Resolution.w;
		_height = hmdDesc.Resolution.h;

		bind_second_context();
		framebuffer_init(context);
		unbind_second_context();

		// Make eye render buffers
		for (int eye = 0; eye < 2; ++eye)
		{
			ovrSizei idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
			eyeRenderTexture[eye] = new TextureBuffer(session, true, true, idealTextureSize, 1, NULL, 1);
			eyeDepthBuffer[eye] = new DepthBuffer(eyeRenderTexture[eye]->GetSize(), 0);

			if (!eyeRenderTexture[eye]->TextureChain)
			{
				if (retryCreate) {
				//	goto Done;
					throw std::runtime_error("Failed to create texturechain.");
				}
			}
		}

		// Setup Window and Graphics
		// Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
		//ovrSizei windowSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };
		ovrSizei windowSize = { _width, _height };

		ovrMirrorTextureDesc desc;
		memset(&desc, 0, sizeof(desc));
		desc.Width = windowSize.w;
		desc.Height = windowSize.h;
		//desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.Format = OVR_FORMAT_R16G16B16A16_FLOAT;

		// Create mirror texture and an FBO used to copy mirror texture to back buffer
		ovrResult result = ovr_CreateMirrorTextureGL(session, &desc, &mirrorTexture);
		if (!OVR_SUCCESS(result))
		{
			if (retryCreate) {
				//goto Done;
				throw std::runtime_error("Create mirror texture failed.");
			}
		}

		bind_second_context();

		// Configure the mirror read buffer
		GLuint texId;
		ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &texId);
		
		glGenFramebuffers(1, &mirrorFBO);
		_glException();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
		_glException();
		glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
		_glException();
		glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
		_glException();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		_glException();

		unbind_second_context();
		
		// Turn off vsync to let the compositor do its magic
		//wglSwapIntervalEXT(0);

		// Make scene - can simplify further if needed
		//roomScene = new Scene(false);

		// FloorLevel will give tracking poses where the floor height is 0
		ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);


		// OCULUS END
		//
		//
		//
		//
		//
		//


		addItem(context->floatImage(_width, _height));
	}

	void oculusDraw::view_thread() {
		while (!view_thread_over) {

			// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
			ovrEyeRenderDesc eyeRenderDesc[2];
			eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
			eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

			// Get eye poses, feeding in correct IPD offset
			ovrPosef                  EyeRenderPose[2];
			ovrVector3f               HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset,
				eyeRenderDesc[1].HmdToEyeOffset };

			double sensorSampleTime;    // sensorSampleTime is fed into the layer later
			ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyeOffset, EyeRenderPose, &sensorSampleTime);


			for (int eye = 0; eye < 2; ++eye) {
				// Get view and projection matrices
				//Matrix4f rollPitchYaw = Matrix4f::RotationY(_camera->get_theta());

				auto quat = EyeRenderPose[eye].Orientation;
				//quat.x = -quat.x;
				//quat.y = -quat.y;
				Matrix4f finalRollPitchYaw = /*rollPitchYaw * */Matrix4f(quat);

				//Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
				//Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
				//auto loc = _camera->get_location();
				//Vector3f shiftedEyePos = Vector3f(loc.x, loc.y, loc.z) + rollPitchYaw.Transform(EyeRenderPose[eye].Position);

				auto tra = finalRollPitchYaw;//.Transposed();

				glm::mat4 all;
				all[0][0] = tra.M[0][0];
				all[0][1] = tra.M[0][1];
				all[0][2] = tra.M[0][2];
				all[0][3] = tra.M[0][3];

				all[1][0] = tra.M[1][0];
				all[1][1] = tra.M[1][1];
				all[1][2] = tra.M[1][2];
				all[1][3] = tra.M[1][3];

				all[2][0] = tra.M[2][0];
				all[2][1] = tra.M[2][1];
				all[2][2] = tra.M[2][2];
				all[2][3] = tra.M[2][3];

				all[3][0] = tra.M[3][0];
				all[3][1] = tra.M[3][1];
				all[3][2] = tra.M[3][2];
				all[3][3] = tra.M[3][3];

//				auto cop = rollPitchYaw.Transform(EyeRenderPose[eye].Position);
				auto cop = EyeRenderPose[eye].Position;

				auto event = std::unique_ptr<azure::Event>(new azure::Event("camera_additional_shift_matrix", 0));
				event->setAttribute<glm::mat4>("matrix", all);
				event->setAttribute<float>("fov", 45);
				event->setAttribute<glm::vec3>("location", glm::vec3(cop.x, cop.y, cop.z));
				azure::Events::Push(std::move(event));
			}

			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}
	}

	void oculusDraw::process() {
		_tick++;

		//_camera_event_handler->frame_tick();
		//_camera_event_handler->move_camera();

		thread_init();

		/*
		_program->use();
		
		_program->uniformV("M", false, &(_camera->get_model())[0][0]);
		
		_program->uniformV("V", false, &(_camera->get_view())[0][0]);
		//    _program->uniformV("MV", false, &MV[0][0]);
		_program->uniformV("MVP", false, &(_camera->get_mvp())[0][0]);
		*/


		/*glEnable(GL_CULL_FACE);
		_glException();
		glCullFace(GL_BACK);
		_glException();
		*/

		//
		//
		// oculus begin

		ovrResult result;
		
		bool retryCreate = false;

		ovrSessionStatus sessionStatus;
		ovr_GetSessionStatus(session, &sessionStatus);
		if (sessionStatus.ShouldQuit)
		{
			// Because the application is requested to quit, should not request retry
			retryCreate = false;
			// break;
			return;
		}
		if (sessionStatus.ShouldRecenter)
			ovr_RecenterTrackingOrigin(session);

//		if (sessionStatus.IsVisible) //this make it show nothing if the headset isn't on. not ideal!
		{
			/*
			// Keyboard inputs to adjust player orientation
			static float Yaw(3.141592f);
			if (Platform.Key[VK_LEFT])  Yaw += 0.02f;
			if (Platform.Key[VK_RIGHT]) Yaw -= 0.02f;

			// Keyboard inputs to adjust player position
			static Vector3f Pos2(0.0f, 0.0f, -5.0f);
			if (Platform.Key['W'] || Platform.Key[VK_UP])     Pos2 += Matrix4f::RotationY(Yaw).Transform(Vector3f(0, 0, -0.05f));
			if (Platform.Key['S'] || Platform.Key[VK_DOWN])   Pos2 += Matrix4f::RotationY(Yaw).Transform(Vector3f(0, 0, +0.05f));
			if (Platform.Key['D'])                            Pos2 += Matrix4f::RotationY(Yaw).Transform(Vector3f(+0.05f, 0, 0));
			if (Platform.Key['A'])                            Pos2 += Matrix4f::RotationY(Yaw).Transform(Vector3f(-0.05f, 0, 0));
			*/
			// Animate the cube
			//static float cubeClock = 0;
			//roomScene->Models[0]->Pos = Vector3f(9 * (float)sin(cubeClock), 3, 9 * (float)cos(cubeClock += 0.015f));

			// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyeOffset) may change at runtime.
			ovrEyeRenderDesc eyeRenderDesc[2];
			eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
			eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

			// Get eye poses, feeding in correct IPD offset
			ovrPosef                  EyeRenderPose[2];
			ovrVector3f               HmdToEyeOffset[2] = { eyeRenderDesc[0].HmdToEyeOffset,
				eyeRenderDesc[1].HmdToEyeOffset };

			double sensorSampleTime;    // sensorSampleTime is fed into the layer later
			ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyeOffset, EyeRenderPose, &sensorSampleTime);

			// Render Scene to Eye Buffers
			for (int eye = 0; eye < 2; ++eye)
			{
				auto input = _input_right.get();
				if (eye == 1) {
					if (_input_left.get() != nullptr) {
						input = _input_left.get();
					}
				}

				if (input.get() != nullptr) {

					cl::Event event3;
					std::vector<cl::Memory> glObjects;
					glObjects.push_back(*_eye_left);
					glObjects.push_back(*_eye_right);

					queue->enqueueAcquireGLObjects(&glObjects, NULL, &event3);
					event3.wait();

					auto ptr = input->outLock();
					if (ptr == nullptr) {
						input = nullptr;
					} else {
						_copy->setArg(0, ptr.get_image());
						if (eye == 0) {
							_copy->setArg(1, *_eye_left);
						} else {
							_copy->setArg(1, *_eye_right);
						}

						cl::Event event;
						queue->enqueueNDRangeKernel(*_copy, cl::NullRange, cl::NDRange(tex_width, tex_height), cl::NullRange, NULL, &event);
						event.wait();

						input->outUnlock();
					}
					cl::Event event2;
					queue->enqueueReleaseGLObjects(&glObjects, NULL, &event2);
					event2.wait();
				}

				// Switch to eye render target
				eyeRenderTexture[eye]->SetAndClearRenderSurface(eyeDepthBuffer[eye]);
				/*
				// Get view and projection matrices
				Matrix4f rollPitchYaw = Matrix4f::RotationY(_camera->get_theta());
				Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(EyeRenderPose[eye].Orientation);
				Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
				Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
				//auto loc = _camera->get_location();
				//Vector3f shiftedEyePos = Vector3f(loc.x, loc.y, loc.z) + rollPitchYaw.Transform(EyeRenderPose[eye].Position);
				

				glm::mat4 all;
				all[0][0] = finalRollPitchYaw.M[0][0];
				all[0][1] = finalRollPitchYaw.M[0][1];
				all[0][2] = finalRollPitchYaw.M[0][2];
				all[0][3] = finalRollPitchYaw.M[0][3];

				all[1][0] = finalRollPitchYaw.M[1][0];
				all[1][1] = finalRollPitchYaw.M[1][1];
				all[1][2] = finalRollPitchYaw.M[1][2];
				all[1][3] = finalRollPitchYaw.M[1][3];

				all[2][0] = finalRollPitchYaw.M[2][0];
				all[2][1] = finalRollPitchYaw.M[2][1];
				all[2][2] = finalRollPitchYaw.M[2][2];
				all[2][3] = finalRollPitchYaw.M[2][3];

				all[3][0] = finalRollPitchYaw.M[3][0];
				all[3][1] = finalRollPitchYaw.M[3][1];
				all[3][2] = finalRollPitchYaw.M[3][2];
				all[3][3] = finalRollPitchYaw.M[3][3];

				auto cop = rollPitchYaw.Transform(EyeRenderPose[eye].Position);
				
				auto event = std::unique_ptr<azure::Event>(new azure::Event("camera_additional_shift_matrix", 0));
				event->setAttribute<glm::mat4>("matrix", all);
				event->setAttribute<float>("fov", 90);
				event->setAttribute<glm::vec3>("location", glm::vec3(cop.x, cop.y, cop.z));
				azure::Events::Push(std::move(event));
				*/
				/*
				Matrix4f view = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
				Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_None);
				
				Matrix4f MVP = proj * view;
				*/
				_program->use();
				//hmdDesc.DefaultEyeFov[eye];
				/*
				auto from = _camera->get_location();

				GLfloat position[] = { from[0], from[1], from[2], 1.0f };

				GLint location;
				if ((location = _program->uniform("light_position")) < 0) { return; }
				glUniform3fv(location, 1, position);
				_glException();

				
				_program->uniformV("M", false, &(_camera->get_model())[0][0]);
				_program->uniformV("V", true, (GLfloat *)&view);
				//    _program->uniformV("MV", false, &MV[0][0]);
				_program->uniformV("MVP", true, (GLfloat *)&MVP);
				*/

				_program->uniform("tex", 0);
				
				_program->uniform("gamma", 1.0f);

				_program->discard();

				// Render world
				//roomScene->Render(view, proj);

				glActiveTexture(GL_TEXTURE0);
				_glException(); 
				if (eye == 0) {
					_left_eye_sprite->render();
				} else {
					_right_eye_sprite->render();
				}

				// Avoids an error when calling SetAndClearRenderSurface during next iteration.
				// Without this, during the next while loop iteration SetAndClearRenderSurface
				// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
				// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
				eyeRenderTexture[eye]->UnsetRenderSurface();

				// Commit changes to the textures so they get picked up frame
				eyeRenderTexture[eye]->Commit();
			}

			// Do distortion rendering, Present and flush/sync

			ovrLayerEyeFov ld;
			ld.Header.Type = ovrLayerType_EyeFov;
			ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

			for (int eye = 0; eye < 2; ++eye) {
				ld.ColorTexture[eye] = eyeRenderTexture[eye]->TextureChain;
				ld.Viewport[eye] = Recti(eyeRenderTexture[eye]->GetSize());
				ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
				ld.RenderPose[eye] = EyeRenderPose[0];
				ld.SensorSampleTime = sensorSampleTime;
			}

			ovrLayerHeader* layers = &ld.Header;
			result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);
			// exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
			if (!OVR_SUCCESS(result)) {
				//goto Done;
				throw std::runtime_error("Submit returned error");
			}

			frameIndex++;
		}


		glViewport(0, 0, _width, _height);
		_glException();

		bind_gl();

		glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
		_glException();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _frame->get_framebuffer());
		_glException();
		GLint w = _width;
		GLint h = _height;
		glBlitFramebuffer(0, 0, w, h,
			0, 0, w, h,
			GL_COLOR_BUFFER_BIT, GL_NEAREST);
		_glException();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		_glException();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		_glException();

		_frame->bind(false);


		unbind_gl();


		process_gl(openglProcess::postprocess_kernel::copy);
	}


	void oculusDraw::release_local_gl_assets() {

		release_gl_assets();
	}

}
