//
//  newHDRProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 21/01/2015.
//
//

#ifndef video_mush_newHDRProcessor_hpp
#define video_mush_newHDRProcessor_hpp

#include <Mush Core/imageProcessor.hpp>
#include <Mush Core/imageProcess.hpp>
#include "bt709luminanceProcess.hpp"
#include <Mush Core/fixedExposureProcess.hpp>
#include "nuHDRProcess.hpp"

class nuHDRProcessor : public mush::imageProcessor {
public:
    nuHDRProcessor(float darken) : mush::imageProcessor() {
        
    }
    
    ~nuHDRProcessor() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        imageBuf = castToImage(buffers.begin()[0]);
        
        exposure = make_shared<mush::fixedExposureProcess>(darken);
        exposure->init(context, imageBuf);
        
        bt709 = make_shared<mush::bt709luminanceProcess>();
        bt709->init(context, exposure);
        
        nuHDR = make_shared<nuHDRProcess>();
        nuHDR->init(context, {exposure, bt709});
        
        _guiBuffers.push_back(exposure);
        _guiBuffers.push_back(bt709);
        _guiBuffers.push_back(nuHDR);
        
    }
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const {
        return {nuHDR, bt709};
    }
    
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() {
        const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
        _guiBuffers.clear();
        return buffs;
    }
    
    void process() {
        exposure->process();
        bt709->process();
        nuHDR->process();
    }
    
    void go() {
        while (imageBuf->good()) {
            process();
        }
        
        bt709->release();
        nuHDR->release();
    }
    
private:
    std::shared_ptr<mush::imageBuffer> imageBuf = nullptr;
    std::shared_ptr<mush::imageProcess> exposure = nullptr;
    std::shared_ptr<mush::imageProcess> bt709 = nullptr;
    std::shared_ptr<mush::imageProcess> nuHDR = nullptr;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
    float darken = 0.0f;
};

#endif
