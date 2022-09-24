//
//  trayraceProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 04/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_trayraceProcessor_hpp
#define video_mush_trayraceProcessor_hpp

#ifdef __APPLE__
#include "dll.hpp"
#else
#include "../dll.hpp"
#endif

namespace mush {
    class imageProcess;
    class imageBuffer;
    class ringBuffer;
    class integerMapProcess;
    class frameStepper;
    class imageProcessor;
    class quitEventHandler;
    class ffmpegEncodeDecode;
    class psnrProcess;
    class ssimProcess;
    class timerWrapper;
}

class edgeEncodingProcessor;
class getDiscontinuities;
class trayraceRedraw;
class trayraceUpsample;
class trayraceCompose;

#include <Mush Core/opencl.hpp>
#include <Mush Core/timerWrapper.hpp>
#include <Mush Core/imageProcessor.hpp>

#include <thread>

class VIDEOMUSH_EXPORTS trayraceProcessor : public mush::imageProcessor {
public:
    trayraceProcessor(std::shared_ptr<mush::imageProcess> parRast);
    
    ~trayraceProcessor();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
	static const std::vector<std::string> listKernels();
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
    std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;
    std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const override;
    
    void go() override;
                                  
    std::shared_ptr<trayraceRedraw> getRedraw();
    
private:
	shared_ptr<mush::opencl> context = nullptr;
	shared_ptr <mush::ringBuffer> _viewport = nullptr;
    shared_ptr <mush::imageProcess> parRast  = nullptr;
    
    shared_ptr <mush::imageProcess> depthMult  = nullptr;
    
    shared_ptr <edgeEncodingProcessor> depthEdge  = nullptr;
    shared_ptr <edgeEncodingProcessor> motionEdge  = nullptr;
    shared_ptr <mush::ringBuffer> motionReconstruction = nullptr;
    
    shared_ptr <mush::imageProcess> depthDelay  = nullptr;
    
    shared_ptr <mush::integerMapProcess> discontinuities  = nullptr;
    shared_ptr <trayraceRedraw> redraw  = nullptr;
    shared_ptr <mush::imageProcess> upsample  = nullptr;
    shared_ptr <mush::imageProcess> compose  = nullptr;
    
    shared_ptr <mush::ffmpegEncodeDecode> ffmpegED  = nullptr;
    shared_ptr <mush::imageProcess> scaleImage  = nullptr;

	shared_ptr <mush::imageProcess> ffmpegED_upscale = nullptr;
    
    shared_ptr <mush::imageProcess> temp  = nullptr;
    shared_ptr <mush::ffmpegEncodeDecode> ffmpegED_full  = nullptr;
    
    shared_ptr <mush::imageProcess> switcher  = nullptr;
    //shared_ptr <mush::imageProcess> testsw  = nullptr;
    //shared_ptr <mush::imageProcess> testsw2  = nullptr;

	shared_ptr <mush::imageProcess> _motion_reprojection = nullptr;
    
	std::vector<std::shared_ptr<mush::imageBuffer>> _nulls;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> _guiBuffers;
    
    std::vector<std::shared_ptr<mush::frameStepper>> steppers;
    
    std::shared_ptr<mush::quitEventHandler> quitHandler = nullptr;
    
    shared_ptr <mush::imageProcess> _output  = nullptr;
    
    std::shared_ptr<mush::timerWrapper> _timer = nullptr;
    
};

#endif
