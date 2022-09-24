#ifndef HDRTONEMAP_HPP
#define HDRTONEMAP_HPP

#include "tonemapProcess.hpp"
#include <Mush Core/imageProcessor.hpp>

class tonemapEncoder : public mush::imageProcessor {
public:
	tonemapEncoder() {

	}

	~tonemapEncoder() {}
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        profileInit();
        
        if (buffers.size() < 1) {
            putLog("No buffers at encoder");
        }
        inputBuffer = buffers.begin()[0];
        
        tonemap = make_shared<tonemapProcess>();
        tonemap->init(context, inputBuffer);
        
		queue = context->getQueue();
        
        _processes.push_back(tonemap);
        
        _guiBuffers.push_back(tonemap);
        _guiBuffers.push_back(castToImage(inputBuffer));
    }
    
	void process() {

		profile.inReadStart();
		profile.inReadStop();

		profile.executionStart();
		
		profile.tonemapStart();
        for (auto pro : _processes) {
            pro->process();
        }
		profile.tonemapStop();

		profile.chromaStop();

		profile.executionStop();

		profile.readFromGPUStart();
        
		profile.readFromGPUStop();

		profile.waitStart();
        
		profile.waitStop();

		profile.stop();

		profile.report();
	}


	void profileInit() {
		profile.init();
	}

	void profileReport() {
		profile.finalReport();
	}

    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const {
        return {tonemap};
    }
    
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() {
        const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
        _guiBuffers.clear();
        return buffs;
    }
    
    void go() {
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

private:
	shared_ptr<mush::opencl> context = nullptr;
	shared_ptr<mush::ringBuffer> inputBuffer = nullptr;
    
    std::shared_ptr<mush::imageProcess> tonemap = nullptr;
    
    std::vector<std::shared_ptr<mush::processNode>> _processes;
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;

	cl::Event event;
	cl::CommandQueue * queue;

	Profile profile;
};

#endif
