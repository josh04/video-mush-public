//
//  waveformProcessor.cpp
//  video-mush
//
//  Created by Josh McNamee on 14/10/2014.
//
//

extern void SetThreadName(const char*);

#include "waveformProcess.hpp"

#include "waveformProcessor.hpp"

#include "waveformCombiner.hpp"
#include <Mush Core/fixedExposureProcess.hpp>
#include "nullProcess.hpp"
#include <Mush Core/frameStepper.hpp>
#include <azure/eventkey.hpp>
#include "laplaceProcess.hpp"

void waveformProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    profileInit();
    
    this->context = context;
    
    if (buffers.size() < 1) {
        throw std::runtime_error("No buffers at encoder");
    }

    inputBuffer = buffers.begin()[0];

	auto image = castToImage(inputBuffer);
	if (image != nullptr) {
		_guiBuffers.push_back(image);
	}

    if (buffers.size() > 1) {
        _guiBuffers.push_back(std::dynamic_pointer_cast<mush::guiAccessible>(buffers.begin()[1]));
    }
    /*
    exposure = make_shared<mush::fixedExposureProcess>(darken);
    exposure->init(context, inputBuffer);
    unsigned int height, size;
    exposure->getParams(width, height, size);
	exposure->setTagInGuiName(std::dynamic_pointer_cast<mush::guiAccessible>(inputBuffer)->getTagInGuiName());
    
    nuller = make_shared<mush::nullProcess>();
    
    //_guiBuffers.push_back(exposure);
    */
    
    if (waveformMode == mush::waveformMode::luma) {
        waveformProcLuma = make_shared<waveformProcess>(_waveform_channel::luma);
        waveformProcLuma->init(context, inputBuffer);
		waveformProcLuma->setTagInGuiName("HDR Waveform");
        //nuller->init(context, waveformProcLuma);
        _guiBuffers.push_back(waveformProcLuma);
    }
    
    if (waveformMode == mush::waveformMode::r) {
        waveformProcR = make_shared<waveformProcess>(_waveform_channel::r);
		waveformProcR->init(context, inputBuffer);
		waveformProcR->setTagInGuiName("HDR Waveform");
        //nuller->init(context, waveformProcR);
        
        _guiBuffers.push_back(waveformProcR);
    }
    
    if (waveformMode == mush::waveformMode::g) {
        waveformProcG = make_shared<waveformProcess>(_waveform_channel::g);
		waveformProcG->init(context, inputBuffer);
		waveformProcG->setTagInGuiName("HDR Waveform");
        //nuller->init(context, waveformProcG);
        
        _guiBuffers.push_back(waveformProcG);
    }
    
    if (waveformMode == mush::waveformMode::b) {
        waveformProcB = make_shared<waveformProcess>(_waveform_channel::b);
		waveformProcB->init(context, inputBuffer);
		waveformProcB->setTagInGuiName("HDR Waveform");
        //nuller->init(context, waveformProcB);
        
        _guiBuffers.push_back(waveformProcB);
    }
    
    if (waveformMode == mush::waveformMode::rgb) {
        waveformProcRGB_R = make_shared<waveformProcess>(_waveform_channel::r);
        waveformProcRGB_R->init(context, inputBuffer);
        waveformProcRGB_G = make_shared<waveformProcess>(_waveform_channel::g);
        waveformProcRGB_G->init(context, inputBuffer);
        waveformProcRGB_B = make_shared<waveformProcess>(_waveform_channel::b);
        waveformProcRGB_B->init(context, inputBuffer);
        
        waveformComb = make_shared<waveformCombiner>();
		waveformComb->init(context, { waveformProcRGB_R, waveformProcRGB_G, waveformProcRGB_B });
		waveformComb->setTagInGuiName("HDR Waveform");
        //nuller->init(context, waveformComb);
        
        _guiBuffers.push_back(waveformComb);
    }
    
    temp = make_shared<mush::fixedExposureProcess>(0.0f);
    temp->init(context, buffers.begin()[0]);

    steppers.push_back(make_shared<mush::frameStepper>());
    
}

void waveformProcessor::process() {
    profile.start();
    if (sw) {
        _switchMode();
        sw = false;
    }
    
    profile.inReadStart();
    //exposure->process();
    profile.inReadStop();
    
    profile.writeToGPUStart();
    
    temp->process();
    
    profile.writeToGPUStop();
    
    profile.executionStart();
    if (waveformMode == mush::waveformMode::luma) {
        waveformProcLuma->process();
    }
    
    if (waveformMode == mush::waveformMode::r) {
        waveformProcR->process();
    }
    
    if (waveformMode == mush::waveformMode::g) {
        waveformProcG->process();
    }
    
    if (waveformMode == mush::waveformMode::b) {
        waveformProcB->process();
    }
    
    if (waveformMode == mush::waveformMode::rgb) {
        waveformProcRGB_R->process();
        waveformProcRGB_G->process();
        waveformProcRGB_B->process();
        waveformComb->process();
    }
    profile.executionStop();
    
    profile.writeStart();
    //nuller->process();
    profile.writeStop();
    
    profile.readFromGPUStart();
    steppers[0]->process();
    profile.readFromGPUStop();
    
    profile.waitStart();
    profile.waitStop();
    
    profile.stop();
    profile.report();
}

void waveformProcessor::switchMode() {
    sw = true;
}


void waveformProcessor::profileInit() {
    profile.init();
}

void waveformProcessor::profileReport() {
    profile.finalReport();
}

const std::vector<std::shared_ptr<mush::ringBuffer>> waveformProcessor::getBuffers() const {
    return {temp};
}

std::vector<std::shared_ptr<mush::guiAccessible>> waveformProcessor::getGuiBuffers() {
    const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
    _guiBuffers.clear();
    return buffs;
}

std::vector<std::shared_ptr<mush::frameStepper>> waveformProcessor::getFrameSteppers() const {
    return steppers;
}

void waveformProcessor::go() {
    SetThreadName("waveform");
    
    while (inputBuffer->good()) {
        process();
    }
    /*
    if (exposure != nullptr) {
        exposure->release();
    }
    */
    if (waveformProcLuma != nullptr) {
        waveformProcLuma->release();
    }
    if (waveformProcRGB_R != nullptr) {
        waveformProcRGB_R->release();
    }
    if (waveformProcRGB_G != nullptr) {
        waveformProcRGB_G->release();
    }
    if (waveformProcRGB_B != nullptr) {
        waveformProcRGB_B->release();
    }
    if (waveformProcR != nullptr) {
        waveformProcR->release();
    }
    if (waveformProcG != nullptr) {
        waveformProcG->release();
    }
    if (waveformProcB != nullptr) {
        waveformProcB->release();
    }
    /*
    if (nuller != nullptr) {
        nuller->release();
    }
    */
    if (temp != nullptr) {
        temp->release();
    }
    
    if (steppers[0] != nullptr) {
        steppers[0]->release();
    }
}

void waveformProcessor::_switchMode() {
    
    inputBuffer->removeRepeat();
    
    if (waveformMode == mush::waveformMode::luma) {
        waveformMode = mush::waveformMode::rgb;
        
        if (waveformProcRGB_R == nullptr) {
            waveformProcRGB_R = make_shared<waveformProcess>(_waveform_channel::r);
            waveformProcRGB_R->init(context, inputBuffer);
        }
        if (waveformProcRGB_G == nullptr) {
            waveformProcRGB_G = make_shared<waveformProcess>(_waveform_channel::g);
            waveformProcRGB_G->init(context, inputBuffer);
        }
        if (waveformProcRGB_B == nullptr) {
            waveformProcRGB_B = make_shared<waveformProcess>(_waveform_channel::b);
            waveformProcRGB_B->init(context, inputBuffer);
        }
        
        if (waveformComb == nullptr) {
            waveformComb = make_shared<waveformCombiner>();
            waveformComb->init(context, {waveformProcRGB_R, waveformProcRGB_G, waveformProcRGB_B});
        }
        //nuller->reInit(waveformComb);
        waveformProcLuma->passGuiTag(waveformComb);
        return;
    }
    
    if (waveformMode == mush::waveformMode::rgb) {
        inputBuffer->removeRepeat();
        inputBuffer->removeRepeat();
        waveformMode = mush::waveformMode::r;
        if (waveformProcR == nullptr) {
            waveformProcR = make_shared<waveformProcess>(_waveform_channel::r);
            waveformProcR->init(context, inputBuffer);
        }
        //nuller->reInit(waveformProcR);
        waveformComb->passGuiTag(waveformProcR);
        return;
    }
    
    if (waveformMode == mush::waveformMode::r) {
        waveformMode = mush::waveformMode::g;
        if (waveformProcG == nullptr) {
            waveformProcG = make_shared<waveformProcess>(_waveform_channel::g);
            waveformProcG->init(context, inputBuffer);
        }
        //nuller->reInit(waveformProcG);
        waveformProcR->passGuiTag(waveformProcG);
        return;
    }
    
    if (waveformMode == mush::waveformMode::g) {
        waveformMode = mush::waveformMode::b;
        if (waveformProcB == nullptr) {
            waveformProcB = make_shared<waveformProcess>(_waveform_channel::b);
            waveformProcB->init(context, inputBuffer);
        }
        //nuller->reInit(waveformProcB);
        waveformProcG->passGuiTag(waveformProcB);
        return;
    }
    
    if (waveformMode == mush::waveformMode::b) {
        waveformMode = mush::waveformMode::luma;
        if (waveformProcLuma == nullptr) {
            waveformProcLuma = make_shared<waveformProcess>(_waveform_channel::luma);
            waveformProcLuma->init(context, inputBuffer);
        }
        //nuller->reInit(waveformProcLuma);
        waveformProcB->passGuiTag(waveformProcLuma);
        return;
    }
    
}

bool waveformProcessor::event(std::shared_ptr<azure::Event> event) {
	if (event->isType("keyDown")) {
		if (event->getAttribute<azure::Key>("key") == azure::Key::y) {
			switchMode();
		}
	}

	return false;
}
