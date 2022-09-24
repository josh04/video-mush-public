#ifndef oculusVideoPlayer_HPP
#define oculusVideoPlayer_HPP

#include <OVR_CAPI.h>

#include <array>
#include <azure/framebuffer.hpp>
#include <azure/program.hpp>

#include <Mush Core/opencl.hpp>
#include <Mush Core/openglProcess.hpp>
#include <Mush Core/registerContainer.hpp>

#include <Mush Core/sphereGL.hpp>
#include <Mush Core/camera_event_handler.hpp>
#include <Mush Core/camera_base.hpp>

struct DepthBuffer;
struct TextureBuffer;

namespace mush {
    
    class oculusVideoPlayer : public openglProcess {
    public:
		oculusVideoPlayer();
        ~oculusVideoPlayer();
        
        void release() override;
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        void process() override;
        void release_local_gl_assets();
        
    private:
        mush::registerContainer<mush::imageBuffer> _input;
		mush::registerContainer<mush::imageBuffer> _input2;
        
        std::shared_ptr<mush::camera::base> _camera = nullptr;
        std::shared_ptr<mush::camera::camera_event_handler> _camera_event_handler = nullptr;
        
        std::shared_ptr<azure::Program> _program = nullptr;
        std::shared_ptr<azure::Texture> _texture = nullptr;
		std::shared_ptr<azure::Texture> _texture2 = nullptr;
        std::shared_ptr<sphereGL> _sphere = nullptr;
        
        cl::ImageGL * _input_texture = nullptr;
		cl::ImageGL * _input_texture2 = nullptr;
        
        float count = 0.0f;
        
        float _fps = 25.0f;
        
        uint64_t _tick = 0;
        
        float _speed = 20.0f;
        
        unsigned int tex_width, tex_height;



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