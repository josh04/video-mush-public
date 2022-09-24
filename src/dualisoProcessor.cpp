//
//  dualisoProcessor.cpp
//  video-mush
//
//  Created by Josh McNamee on 28/11/2015.
//
//

#include "dualisoProcessor.hpp"

#include "debayerProcess.hpp"
#include "quarterImageProcess.hpp"
#include "whitePointProcess.hpp"
#include "dualisoUpsize.hpp"
#include "dualisoScale.hpp"
#include "dualisoMerge.hpp"
#include "dualisoPadImage.hpp"
#include "dualisoClipImage.hpp"
#include "dualisoNeaten.hpp"
#include <Mush Core/fixedExposureProcess.hpp>
#include "bayerGaussianProcess.hpp"
#include "bayerSharpenProcess.hpp"
#include "dualisoWeight.hpp"

#include <math.h>
#include <sstream>
#include <azure/eventkey.hpp>

using namespace mush;
extern void SetThreadName(const char * threadName);

dualisoProcessor::dualisoProcessor(float dual_iso_comp_factor) : imageProcessor(), azure::Eventable(), fac(dual_iso_comp_factor) {
    
}

void dualisoProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 1);
    _input_buffer = castToImage(buffers.begin()[0]);
    
    _pad_image = std::make_shared<dualiso_pad_image>();
    _pad_image->init(context, _input_buffer);
    
    _weight = std::make_shared<dualiso_weight>(0);
    _weight->init(context, _pad_image);
    _weight->setTagInGuiName("Dual ISO Weight");
    _guiBuffers.push_back(_weight);
    
    videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(_weight));
    
    _upsize_dark = std::make_shared<dualiso_upsize>(1, 0.0f);
    //_input_buffer->addRepeat();
    _upsize_dark->init(context, _weight);
    _upsize_dark->setTagInGuiName("Dual ISO Dark Upsize");
    _guiBuffers.push_back(_upsize_dark);
    
    _upsize_light = std::make_shared<dualiso_upsize>(0, 0.0f);
    _upsize_light->init(context, _weight);
    _upsize_light->setTagInGuiName("Dual ISO Light Upsize");
    
    _upsize_light_scale = std::make_shared<mush::fixedExposureProcess>(-fac);
    _upsize_light_scale->init(context, {_upsize_light});
    
    _upsize_light_scale->setTagInGuiName("Dual ISO Light Upsize");
    _guiBuffers.push_back(_upsize_light_scale);
    
    
    _scale_light = std::make_shared<dualiso_scale>(1, fac);
    _scale_light->init(context, {_weight});
    _scale_light->setTagInGuiName("Dual ISO Light Scale");
    
    
    _scale_light_scale = std::make_shared<mush::fixedExposureProcess>(-fac);
    _scale_light_scale->init(context, {_scale_light});
    
    _scale_light_scale->setTagInGuiName("Dual ISO Light Scale");
    _guiBuffers.push_back(_scale_light_scale);
    
    _merge = std::make_shared<dualiso_merge>(fac);
    _merge->init(context, {_upsize_dark, _upsize_light_scale, _scale_light_scale});
    _merge->setTagInGuiName("Dual ISO Merge");
    _guiBuffers.push_back(_merge);
    /*
    _gauss = std::make_shared<mush::bayerGaussianProcess>(bayerGaussianProcess::type::vertical, 0.5f, 2);
    _gauss->init(context, _merge);
    
    _gauss2 = std::make_shared<mush::bayerGaussianProcess>(bayerGaussianProcess::type::horizontal, 0.2f, 2);
    _gauss2->init(context, _gauss);
    
    _gauss2->setTagInGuiName("Dual ISO Gaussian");
    _guiBuffers.push_back(_gauss2);
    */
    _sharp = std::make_shared<bayerSharpenProcess>();
    _sharp->init(context, _merge);
    
    _sharp->setTagInGuiName("Dual ISO Sharpen");
    _guiBuffers.push_back(_sharp);
    
    
    _neaten = std::make_shared<dualiso_neaten>(1.1f, 1);
    _neaten->init(context, _sharp);
    _neaten->setTagInGuiName("Dual ISO Neaten");
    _guiBuffers.push_back(_neaten);
    
    _merge_scale = std::make_shared<mush::fixedExposureProcess>(fac);
    _merge_scale->init(context, {_neaten});
    
    _clip_image = std::make_shared<dualiso_clip_image>();
    _clip_image->init(context, _merge_scale);
    
    /*
    _debayer1 = make_shared<debayerProcess>();
    _debayer1->setTagInGuiName("Dark Upsize Debayer");
    _debayer1->init(context, _upsize_dark);
     
    _debayer2 = make_shared<debayerProcess>();
    _debayer2->setTagInGuiName("Light Upsize Debayer");
    _debayer2->init(context, _upsize_light);
    
    _debayer3 = make_shared<debayerProcess>();
    _debayer3->setTagInGuiName("Dark Scale Debayer");
    _debayer3->init(context, _scale_dark);
    
    _debayer4 = make_shared<debayerProcess>();
    _debayer4->setTagInGuiName("Light Scale Debayer");
    _debayer4->init(context, _scale_light);
    
    _guiBuffers.push_back(_debayer1);
    _guiBuffers.push_back(_debayer2);
    _guiBuffers.push_back(_debayer3);
    _guiBuffers.push_back(_debayer4);
     
    videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(_debayer1));
    videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(_debayer2));
    videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(_debayer3));
    videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(_debayer4));
     */
    
}

const std::vector<std::shared_ptr<mush::ringBuffer>> dualisoProcessor::getBuffers() const {
    return {_clip_image};
}

std::vector<std::shared_ptr<mush::guiAccessible>> dualisoProcessor::getGuiBuffers() {
    const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
    _guiBuffers.clear();
    return buffs;
}

void dualisoProcessor::process() {
    _pad_image->process();
    _weight->process();
    _upsize_dark->process();
    _upsize_light->process();
    _upsize_light_scale->process();
    _scale_light->process();
    _scale_light_scale->process();
    _merge->process();
    /*_gauss->process();
    _gauss2->process();*/
    _sharp->process();

    _neaten->process();
    _merge_scale->process();
    _clip_image->process();
    /*
    _debayer1->process();
    _debayer2->process();
    _debayer3->process();
    _debayer4->process();*/
}

void dualisoProcessor::release() {
    _pad_image->release();
    _upsize_dark->release();
    _upsize_light->release();
    _scale_light->release();
    _neaten->release();
    _merge->release();
    /*_gauss->release();
    _gauss2->release();
    _sharp->release();*/
    _merge_scale->release();
    _clip_image->release();
}

void dualisoProcessor::go() {
    SetThreadName("dualiso");
    while (_input_buffer->good()) {
        process();
    }
    
    release();
}

std::vector<std::shared_ptr<mush::frameStepper>> dualisoProcessor::getFrameSteppers() const {
    return {};
}


bool dualisoProcessor::event(std::shared_ptr<azure::Event> event) {
    std::stringstream strm;
    bool fac_changed = false;
    if (event->isType("keyDown")) {
        auto key = event->getAttribute<azure::Key>("key");
        if (key == azure::Key::p) {
            fac += 0.1f;
            fac_changed = true;
        } else if (key == azure::Key::o) {
            fac -= 0.1f;
            fac_changed = true;
        }
    }
    
    if (fac_changed) {
        
        strm << "Dual ISO Factor changed to " << fac << ".";
        putLog(strm.str());
        _upsize_light_scale->set(-fac);
        _scale_light->set(fac);
        _scale_light_scale->set(-fac);
        _merge->set(fac);
        _merge_scale->set(fac);
    }
    
    return false;
}