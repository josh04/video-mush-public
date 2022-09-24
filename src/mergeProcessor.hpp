//
//  mergeProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 06/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_mergeProcessor_hpp
#define video_mush_mergeProcessor_hpp

#include <Mush Core/fixedExposureProcess.hpp>
#include <Mush Core/imageProcessor.hpp>
#include "mergeTwoProcess.hpp"
#include "mergeThreeProcess.hpp"

#include <thread>

class mergeProcessor : public mush::imageProcessor {
public:
	mergeProcessor(float * isos, float gamma, int exposures) :
	isos(isos), gamma(gamma), exposures(exposures) {
        
	}
    
	~mergeProcessor() {}
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        
        if (buffers.size() < 1) {
            putLog("No buffers at merge");
        }
        inputBuffer = std::dynamic_pointer_cast<mush::guiAccessible>(buffers.begin()[0]);
        
        std::initializer_list<std::shared_ptr<mush::ringBuffer>> list;
        if (exposures == 2) {
            one = make_shared<mush::fixedExposureProcess>(0.0f);
            one->init(context, inputBuffer);
            two = make_shared<mush::fixedExposureProcess>(0.0f);
            two->init(context, inputBuffer);

			inputBuffer->removeRepeat();
            
            _guiBuffers.push_back(one);
            one->setTagInGuiName("First Merge");
            _guiBuffers.push_back(two);
            two->setTagInGuiName("Second Merge");
            
            merge = make_shared<mergeTwoProcess>(isos);
            
            list = {one, two};
        } else if (exposures == 3) {
            one = make_shared<mush::fixedExposureProcess>(0.0f);
            one->init(context, inputBuffer);
            two = make_shared<mush::fixedExposureProcess>(0.0f);
            two->init(context, inputBuffer);
            three = make_shared<mush::fixedExposureProcess>(0.0f);
            three->init(context, inputBuffer);

			inputBuffer->removeRepeat();
			inputBuffer->removeRepeat();
            
            _guiBuffers.push_back(one);
            one->setTagInGuiName("First Merge");
            _guiBuffers.push_back(two);
            two->setTagInGuiName("Second Merge");
            _guiBuffers.push_back(three);
            three->setTagInGuiName("Third Merge");
            
            merge = make_shared<mergeThreeProcess>(isos);
            
            list = {one, two, three};
        }
        merge->init(context, list);
        merge->setTagInGuiName("HDR Merged Input");
        
		queue = context->getQueue();
    }
    
	static const std::vector<std::string> listKernels() {
		std::vector<std::string> kernels;
		return kernels;
	}
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const {
        return {merge};
    }
    
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() {
        const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
        _guiBuffers.clear();
        return buffs;
    }
    
    void process() {
        if (one != nullptr) {
            one->process();
        }
        if (two != nullptr) {
            two->process();
        }
        if (three != nullptr) {
            three->process();
        }
        merge->process();
    }
    
    void go() {
        SetThreadName("merge");
        while (inputBuffer->good()) {
            process();
		}
        
        if (merge != nullptr) {
            merge->release();
        }
        
    }
    
    void setIsos(const float &iso1, const float &iso2, const float &iso3) {
        if (auto merge2 = std::dynamic_pointer_cast<mergeTwoProcess>(merge)) {
            merge2->setIsos(iso1, iso2, iso3);
        }
        if (auto merge3 = std::dynamic_pointer_cast<mergeThreeProcess>(merge)) {
            merge3->setIsos(iso1, iso2, iso3);
        }
    }
    
    //    std::shared_ptr<guiProcess> gui = nullptr;
private:
	shared_ptr<mush::opencl> context = nullptr;
	shared_ptr<mush::guiAccessible> inputBuffer = nullptr;
    
    shared_ptr<mush::imageProcess> merge  = nullptr;
    
    std::shared_ptr<mush::imageProcess> one = nullptr;
    std::shared_ptr<mush::imageProcess> two = nullptr;
    std::shared_ptr<mush::imageProcess> three = nullptr;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
    
    float * isos = nullptr;
	float gamma = 1.0f;
    int exposures = 0;
    
	cl::Event event;
	cl::CommandQueue * queue;
};

#endif
