//
//  slicProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_slicProcessor_hpp
#define media_encoder_slicProcessor_hpp

#include <Mush Core/SetThreadName.hpp>

#include "slicProcess.hpp"
#include <Mush Core/fixedExposureProcess.hpp>
#include <Mush Core/imageProcessor.hpp>
#include <thread>

class slicProcessor : public mush::imageProcessor {
public:
	slicProcessor(mush::config::slicConfigStruct &config) {
        slic = make_shared<slicProcess>(config, &profile);
	}
    
	~slicProcessor() {}
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        profileInit();
        
        if (buffers.size() < 1) {
            putLog("No buffers at encoder");
        }
        inputBuffer = buffers.begin()[0];
        
        std::shared_ptr <mush::imageProcess> exposure = make_shared<mush::fixedExposureProcess>(0.0f);
        exposure->init(context, inputBuffer);
        
        slic->init(context, exposure);
        
		queue = context->getQueue();
        
        _processes.push_back(exposure);
        _processes.push_back(slic);
        
        _guiBuffers.push_back(slic);
        _guiBuffers.push_back(exposure);
        _guiBuffers.push_back(castToImage(inputBuffer));
    }
    
	void process() {
        for (auto pro : _processes) {
            pro->process();
        }
	}
    
    
	void profileInit() {
		profile.init();
	}
    
	void profileReport() {
		profile.finalReport();
	}
    
	static const std::vector<std::string> listKernels() {
		std::vector<std::string> kernels;
		return kernels;
	}
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const {
        return {slic};
    }
    
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() {
        const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
        _guiBuffers.clear();
        return buffs;
    }
    
    void go() {
        SetThreadName("saliency");
        while (inputBuffer->good()) {
			process();
		}
        
        for (auto img : _processes) {
            auto pop = std::dynamic_pointer_cast<mush::ringBuffer>(img);
            if (pop != nullptr) {
                pop->release();
            }
        }
    }
    
//    std::shared_ptr<guiProcess> gui = nullptr;
private:
	shared_ptr<mush::opencl> context = nullptr;
	shared_ptr<mush::ringBuffer> inputBuffer = nullptr;
    
    std::shared_ptr <mush::imageProcess> slic = nullptr;
    
//    std::shared_ptr<ldrRingBuffer> output = nullptr;
    
    std::vector<std::shared_ptr<mush::processNode>> _processes;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
    
	cl::Event event;
	cl::CommandQueue * queue;
    
	Profile profile;
};

#endif
