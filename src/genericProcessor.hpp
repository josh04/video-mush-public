//
//  barcodeProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 24/02/2015.
//
//

#ifndef video_mush_barcodeProcessor_hpp
#define video_mush_barcodeProcessor_hpp

#include <Mush Core/imageProcessor.hpp>
#include <Mush Core/frameStepper.hpp>

namespace mush {
    class timerWrapper;
}

namespace mush {
    class genericProcessor : public imageProcessor {
    public:
        genericProcessor(genericProcess p = genericProcess::falseColour);
        
        ~genericProcessor();
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
        
        std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;
        
        void process() override;
        
        void go() override;
        
        std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;
        
        private:
        std::shared_ptr<mush::imageBuffer> imageBuf = nullptr;
        std::shared_ptr<mush::imageProcess> _generic = nullptr;
		std::shared_ptr<mush::psnrProcess> _psnr = nullptr;
		std::shared_ptr<mush::imageProcess> _copy2 = nullptr;
        
        
        std::shared_ptr<mush::frameStepper> stepper = nullptr;
        
        std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
        
        genericProcess _process;
        
        std::unique_ptr<mush::timerWrapper> _timer = nullptr;
    };
}



#endif
