//
//  waveformProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 29/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_waveformProcessor_hpp
#define media_encoder_waveformProcessor_hpp

#include <azure/eventable.hpp>
#include <Mush Core/opencl.hpp>
#include "profile.hpp"
#include <Mush Core/imageProcessor.hpp>
#include "ConfigStruct.hpp"
#include <thread>

using std::vector;
using std::shared_ptr;
using std::atomic;

namespace mush {
    class imageProcess;
    class ringBuffer;
    class frameStepper;
    class nullProcess;
}

class waveformProcessor : public mush::imageProcessor, public azure::Eventable {
public:
	waveformProcessor(mush::waveformMode waveformMode) :
		mush::imageProcessor(), 
		azure::Eventable(),
		waveformMode(waveformMode) {
        sw = false;
	}
    
	~waveformProcessor() {
    
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
    
    void process();
    void switchMode();
    
    void profileInit();
    void profileReport();
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const;
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers();
    std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;
    
    void go();

	bool event(std::shared_ptr<azure::Event> event) override;
    
private:
    
    void _switchMode();
    
    mush::waveformMode waveformMode = mush::waveformMode::luma;
	shared_ptr<mush::opencl> context = nullptr;
	shared_ptr<mush::ringBuffer> inputBuffer = nullptr;
    
    shared_ptr <mush::imageProcess> waveformProcLuma = nullptr;
    shared_ptr <mush::imageProcess> waveformProcRGB_R = nullptr;
    shared_ptr <mush::imageProcess> waveformProcRGB_G = nullptr;
    shared_ptr <mush::imageProcess> waveformProcRGB_B = nullptr;
    shared_ptr <mush::imageProcess> waveformProcR = nullptr;
    shared_ptr <mush::imageProcess> waveformProcG = nullptr;
    shared_ptr <mush::imageProcess> waveformProcB = nullptr;
    
	//shared_ptr <mush::imageProcess> exposure = nullptr;
	shared_ptr <mush::imageProcess> temp = nullptr; // windows dblbuf poroblem FIXME
    
    vector<shared_ptr<mush::frameStepper>> steppers;
    
    shared_ptr<mush::imageProcess> waveformComb = nullptr;
    //shared_ptr<mush::nullProcess> nuller = nullptr;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
  
    
	Profile profile;
    atomic<bool> sw;
    unsigned int width;
};

#endif
