//
//  dualisoWeight.hpp
//  video-mush
//
//  Created by Josh McNamee on 30/11/2015.
//
//

#ifndef dualisoWeight_h
#define dualisoWeight_h

#include <azure/Eventable.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class tagInGui;

namespace cl {
	class CommandQueue;
	class Image2D;
	class Kernel;
}

namespace mush {
	class opencl;

    class dualiso_weight : public mush::imageProcess, public azure::Eventable {
    public:
		dualiso_weight(int mod);
        
        ~dualiso_weight() {
            
        }
        
		void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
		void process() override;
        
		bool event(std::shared_ptr<azure::Event> event) override;
        
		void inUnlock() override;
        
		void guiTag(int i, tagInGui * tag) override;
        
		void create_temp_image(std::shared_ptr<opencl> context);
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * _weight = nullptr;
        
        mush::registerContainer<mush::imageBuffer> buffer;
        uint8_t _mod = 0;
        
        float _wb_red = 1.0f, _wb_blue = 1.0f;
        
        std::mutex _lock;
        cl::Image2D * _weight_image = nullptr;
        cl::Kernel * _weight_display = nullptr;
        
    };
    
}


#endif /* dualisoWeight_h */
