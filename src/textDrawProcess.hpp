//
//  textDrawProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 05/09/2016.
//
//

#ifndef textDrawProcess_hpp
#define textDrawProcess_hpp

#include <array>
#include <azure/framebuffer.hpp>

#include <Mush Core/opencl.hpp>
#include <Mush Core/openglProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class imguiDrawer;
class imguiEventHandler;

namespace mush {
    
    class textDrawProcess : public openglProcess {
    public:
        textDrawProcess(std::string text, unsigned int width, unsigned int height, const char * resource_dir, std::array<float, 4> bg_colour, std::array<float, 4> text_colour);
        ~textDrawProcess();
        
        void release() override;
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        void process() override;
        void release_local_gl_assets();
        
    private:
        mush::registerContainer<mush::imageBuffer> _input;
        
        std::shared_ptr<imguiDrawer> _imgui_drawer = nullptr;
        std::shared_ptr<imguiEventHandler> _imgui_event_handler = nullptr;
        
        
        float count = 0.0f;
        
        float _fps = 25.0f;
        
        uint64_t _tick = 0;
        
        std::string _text;
        std::string _cursor = "";
        
        const char * _resource_dir;
        
        float _speed = 20.0f;
        
        std::array<float, 4> _text_colour;
        
    };
    
}

#endif /* textDrawProcess_hpp */
