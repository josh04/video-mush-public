//
//  dualisoProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 28/11/2015.
//
//

#ifndef dualisoProcessor_hpp
#define dualisoProcessor_hpp

#include <azure/eventable.hpp>
#include "ConfigStruct.hpp"
#include <Mush Core/imageProcessor.hpp>

namespace mush {
    class imageProcess;
    class frameStepper;
    class imageBuffer;
    class guiAccessible;
    class fixedExposureProcess;
    class dualiso_scale;
    class dualiso_merge;
}

namespace mush {
    class dualisoProcessor : public imageProcessor, public azure::Eventable {
    public:
        dualisoProcessor(float dual_iso_comp_factor);
        
        ~dualisoProcessor() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
        
        std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;
        
        void process() override;
        
        void go() override;
        
        void release();
        
        std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;
        
        bool event(std::shared_ptr<azure::Event> event) override;
        
    private:
        std::shared_ptr<mush::imageBuffer> _input_buffer = nullptr;
        std::shared_ptr<mush::imageProcess> _pad_image = nullptr;
        std::shared_ptr<mush::imageProcess> _weight = nullptr;
        std::shared_ptr<mush::imageProcess> _upsize_dark = nullptr;
        std::shared_ptr<mush::imageProcess> _upsize_light = nullptr;
        
        std::shared_ptr<mush::fixedExposureProcess> _upsize_light_scale = nullptr;
        
        std::shared_ptr<mush::dualiso_scale> _scale_light = nullptr;
        std::shared_ptr<mush::fixedExposureProcess> _scale_light_scale = nullptr;
        std::shared_ptr<mush::imageProcess> _gauss = nullptr;
        std::shared_ptr<mush::imageProcess> _gauss2 = nullptr;
        std::shared_ptr<mush::imageProcess> _sharp = nullptr;
        
        std::shared_ptr<mush::imageProcess> _clip_image = nullptr;
        
        std::shared_ptr<mush::dualiso_merge> _merge = nullptr;
        std::shared_ptr<mush::imageProcess> _neaten = nullptr;
        std::shared_ptr<mush::fixedExposureProcess> _merge_scale = nullptr;
        
        /*
        std::shared_ptr<mush::imageProcess> _debayer1 = nullptr;
        std::shared_ptr<mush::imageProcess> _debayer2 = nullptr;
        std::shared_ptr<mush::imageProcess> _debayer3 = nullptr;
        std::shared_ptr<mush::imageProcess> _debayer4 = nullptr;
        */
        
        std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
        
        float fac = 2.8f;
    };
}

#endif /* dualisoProcessor_hpp */
