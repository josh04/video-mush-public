#ifndef oculusDraw_HPP
#define oculusDraw_HPP

#include <OVR_CAPI.h>

#include <array>
#include <azure/framebuffer.hpp>
#include <azure/program.hpp>
#include "oculusEyeSprite.hpp"

#include <Mush Core/opencl.hpp>
#include <Mush Core/openglProcess.hpp>
#include <Mush Core/registerContainer.hpp>

#include <Mush Core/sphereGL.hpp>
#include <Mush Core/camera_event_handler.hpp>
#include <Mush Core/camera_base.hpp>

struct DepthBuffer;
struct TextureBuffer;

namespace mush {
    
    class oculusDraw : public openglProcess {
    public:
		oculusDraw();
        ~oculusDraw();
        
        void release() override;
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
		void view_thread();

        void process() override;
        void release_local_gl_assets();
        
    private:
        mush::registerContainer<mush::imageBuffer> _input_left;
		mush::registerContainer<mush::imageBuffer> _input_right;

        
        //std::shared_ptr<mush::camera::base> _camera = nullptr;
        //std::shared_ptr<mush::camera::camera_event_handler> _camera_event_handler = nullptr;
        
        std::shared_ptr<azure::Program> _program = nullptr;
        //std::shared_ptr<azure::Texture> _texture = nullptr;

		std::shared_ptr<oculusEyeSprite> _left_eye_sprite = nullptr;
		std::shared_ptr<oculusEyeSprite> _right_eye_sprite = nullptr;
        //std::shared_ptr<sphereGL> _sphere = nullptr;
        
        cl::ImageGL * _eye_left = nullptr;
		cl::ImageGL * _eye_right = nullptr;
        
        float count = 0.0f;
        
        float _fps = 25.0f;
        
        uint64_t _tick = 0;
        
        float _speed = 20.0f;
        
        unsigned int tex_width, tex_height;

		std::atomic_bool view_thread_over;

		TextureBuffer * eyeRenderTexture[2] = { nullptr, nullptr };
		DepthBuffer   * eyeDepthBuffer[2] = { nullptr, nullptr };
		ovrMirrorTexture mirrorTexture = nullptr;
		GLuint          mirrorFBO = 0;
		long long frameIndex = 0;

		ovrSession session;
		ovrGraphicsLuid luid;

		ovrHmdDesc hmdDesc;


    };
    
}

#endif