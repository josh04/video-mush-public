//
//  demoMode.cpp
//  video-mush
//
//  Created by Josh McNamee on 08/08/2015.
//
//

#include "demoMode.hpp"
#include "demoProcess.hpp"
#include "exports.hpp"

using namespace mush;
extern void SetThreadName(const char * threadName);

demoMode::demoMode() : imageProcessor() {
    
}

void demoMode::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer> > buffers) {
    assert(buffers.size() == 1);
    _input_buffer = castToImage(buffers.begin()[0]);
    
    _demo_process = make_shared<demoProcess>(24, 24);
    _demo_process->setTagInGuiName("Demo Controller");
    videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(_demo_process));
    _demo_process->init(context, {_input_buffer});
    
    _guiBuffers.push_back(_demo_process);
}

const std::vector<std::shared_ptr<mush::ringBuffer>> demoMode::getBuffers() const {
    return {_demo_process};
}

std::vector<std::shared_ptr<mush::guiAccessible>> demoMode::getGuiBuffers() {
    const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
    _guiBuffers.clear();
    return buffs;
}

void demoMode::process() {
    _demo_process->process();
}

void demoMode::go() {
    SetThreadName("demoMode");
    while (_input_buffer->good()) {
        process();
    }
    release();
}

void demoMode::release() {
    _demo_process->release();
}

std::vector<std::shared_ptr<mush::frameStepper>> demoMode::getFrameSteppers() const { return {}; };