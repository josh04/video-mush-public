//
//  debayerProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 05/08/2015.
//
//

#ifndef video_mush_debayerProcessor_hpp
#define video_mush_debayerProcessor_hpp


#include "ConfigStruct.hpp"
#include <Mush Core/imageProcessor.hpp>

#include <OpenEXR/ImathMatrix.h>

namespace mush {
    class imageProcess;
    class frameStepper;
    class imageBuffer;
    class guiAccessible;
    class dualisoProcessor;
}

namespace mush {
    class debayerProcessor : public imageProcessor {
    public:
        debayerProcessor(float * whitePoint, rawCameraType camera_type, bool dualISO, float dual_iso_comp_factor, float raw_clamp);
        
        ~debayerProcessor();
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
        
        std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;
        
        void process() override;
        
        void go() override;
        
        void release();
        
        std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;
        
    private:
        std::shared_ptr<mush::imageBuffer> _input_buffer = nullptr;
        std::shared_ptr<mush::imageProcess> _debayer = nullptr;
        std::shared_ptr<mush::imageProcess> _quarter_image = nullptr;
        std::shared_ptr<mush::imageProcess> _white_point = nullptr;
        std::shared_ptr<mush::imageProcess> _chroma_smooth = nullptr;
        
        std::shared_ptr<mush::imageProcess> _output_dummy = nullptr;
        
        std::shared_ptr<mush::dualisoProcessor> _dual_iso = nullptr;
        
        
        std::shared_ptr<mush::frameStepper> _stepper = nullptr;
        
        std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
        
        genericProcess _process;
        
        float * _whitePoint = nullptr;
        
        bool _dualISO = false;
        float _dual_iso_comp_factor;
        rawCameraType _camera_type;
		float _raw_clamp;
    };
}


#endif
