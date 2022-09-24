//
//  sphereMapProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 05/12/2016.
//
//

#ifndef sphereMapProcess_hpp
#define sphereMapProcess_hpp

#include <array>
#include <azure/framebuffer.hpp>
#include <azure/program.hpp>

#include "mush-core-dll.hpp"
#include "opencl.hpp"
#include "openglProcess.hpp"
#include "registerContainer.hpp"

#include "sphereGL.hpp"
#include "camera_event_handler.hpp"
#include "camera_base.hpp"

namespace mush {
    
    class MUSHEXPORTS_API sphereMapProcess : public openglProcess {
    public:
        sphereMapProcess(unsigned int width, unsigned int height);
        sphereMapProcess(unsigned int width, unsigned int height, bool auto_cam, const char * auto_cam_path, float auto_cam_speed, bool quit_at_end);
        ~sphereMapProcess();
        
        void release() override;
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        void process() override;
        void release_local_gl_assets();
        
        std::shared_ptr<azure::Eventable> get_eventable() const override {
            return _camera_event_handler;
        }

		void frame_tick();
        
    private:
        mush::registerContainer<mush::imageBuffer> _input;
        
        std::shared_ptr<mush::camera::base> _camera = nullptr;
        std::shared_ptr<mush::camera::camera_event_handler> _camera_event_handler = nullptr;
        
        std::shared_ptr<azure::Program> _program = nullptr;
        std::shared_ptr<azure::Texture> _texture = nullptr;
        std::shared_ptr<sphereGL> _sphere = nullptr;
        
        cl::ImageGL * _input_texture = nullptr;
        
        float count = 0.0f;
        
        float _fps = 25.0f;
        
        uint64_t _tick = 0;
        
        float _speed = 20.0f;
        
        unsigned int tex_width, tex_height;
        
        size_t _input_token = -1;
        
        bool auto_cam;
        std::string auto_cam_path;
        float auto_cam_speed;
        bool _quit_at_end = false;
    };
    
}

#endif /* sphereMapProcess_hpp */
