//
//  trayraceProcessor.cpp
//  trayrace
//
//  Created by Josh McNamee on 08/09/2014.
//  Copyright (c) 2014 Josh McNamee. All rights reserved.
//

#include <thread>


#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>
#include <Mush Core/fixedExposureProcess.hpp>

#include <Mush Core/psnrProcess.hpp>

#include <Mush Core/imageProcessor.hpp>
#include <Mush Core/frameStepper.hpp>

#include <Mush Core/quitEventHandler.hpp>
#include <Mush Core/ssimProcess.hpp>
#include <Mush Core/timerWrapper.hpp>

#ifdef __APPLE__
#include <Video Mush/exports.hpp>
#include <Video Mush/ffmpegEncodeDecode.hpp>
#include <Video Mush/nullProcess.hpp>
#include <Video Mush/laplaceProcess.hpp>
#include <Video Mush/delayProcess.hpp>
#include <Video Mush/switcherProcess.hpp>
#else
#include "../nullProcess.hpp"
#include "../laplaceProcess.hpp"
#include "../delayProcess.hpp"
#include "../switcherProcess.hpp"

#include "../ffmpegEncodeDecode.hpp"
#include "../exports.hpp"
#endif



#include "edgeThreshold.hpp"
#include "diffuseProcess.hpp"
#include "getDiscontinuities.hpp"
#include "trayraceRedraw.hpp"
#include "trayraceUpsample.hpp"
#include "trayraceCompose.hpp"
#include "imageDownScale.hpp"
#include "edgeEncodingProcessor.hpp"
#include "motionReprojection.hpp"
#include "../scaleImageProcess.hpp"

#include "trayraceProcessor.hpp"

#include "../testSwscale.hpp"

extern void SetThreadName(const char * threadName);

trayraceProcessor::trayraceProcessor(std::shared_ptr<mush::imageProcess> parRast) : mush::imageProcessor(), parRast(parRast) {
    
}

trayraceProcessor::~trayraceProcessor() {}

void trayraceProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    
    assert(buffers.size() == 4);
    
    _timer = std::make_shared<mush::timerWrapper>("remote render");
    
    _viewport  = buffers.begin()[0];
    shared_ptr<mush::ringBuffer> _depth  = buffers.begin()[1];
    shared_ptr<mush::ringBuffer> _normals  = buffers.begin()[2];
    shared_ptr<mush::ringBuffer> _motionVectors  = buffers.begin()[3];

	_guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_viewport));
	_guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_depth));
	_guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_normals));
	_guiBuffers.push_back(std::dynamic_pointer_cast<mush::imageBuffer>(_motionVectors));

    // quitter and stepper
    
    quitHandler = std::make_shared<mush::quitEventHandler>();
    videoMushAddEventHandler(quitHandler);
    
    steppers.push_back(std::make_shared<mush::frameStepper>());
    
    //finish
    
    // Construct the discontinuities
    
    depthMult = make_shared<mush::fixedExposureProcess>(0.0f);
    depthMult->init(context, _depth);
    
    depthDelay = make_shared<mush::delayProcess>();
    depthDelay->init(context, depthMult);
    
    discontinuities = make_shared<getDiscontinuities>();
    discontinuities->init(context, {_motionVectors, depthDelay });
    
    redraw = make_shared<trayraceRedraw>(2);
    redraw->init(context, {discontinuities});
    
    // finish discontinuities
    
    
    // pack depth into edges and reconstruct
    
    depthEdge = make_shared<edgeEncodingProcessor>(3.0f, _timer, "Depth");
    depthEdge->init(context, {depthMult});
    auto depthReconstruction = depthEdge->getBuffers().begin()[0];
    
    // finish depth
    
    // pack motion into edges and reconstruct
    
    motionEdge = make_shared<edgeEncodingProcessor>(0.5f, _timer, "Motion");
    motionEdge->init(context, {_motionVectors});
    motionReconstruction = motionEdge->getBuffers().begin()[0];
    
    //finish motion
    
    
    
    // compose part-render with upscale and edges
    
    scaleImage = make_shared<imageDownScale>(2);
    scaleImage->init(context, {_viewport, redraw});
    
    temp = make_shared<mush::fixedExposureProcess>(0.0f);
    temp->init(context, {_viewport});
    ffmpegED_full = make_shared<mush::ffmpegEncodeDecode>(avcodec_codec::x264, mush::transfer::rec709, 18);
    ffmpegED_full->init(context, {temp});
    
    
    // pass low-res through h264 and back
    
    ffmpegED = make_shared<mush::ffmpegEncodeDecode>(avcodec_codec::x264, mush::transfer::rec709, 18);
    ffmpegED->init(context, {scaleImage});

	unsigned int w, h, s;
	std::dynamic_pointer_cast<mush::imageBuffer>(_viewport)->getParams(w, h, s);
	ffmpegED_upscale = make_shared<mush::scaleImageProcess>(w, h);
	ffmpegED_upscale->init(context, { ffmpegED });
    
    // finish h264
    
    // spatial upsampling, binds to edges of depth, normals
    
    upsample = make_shared<trayraceUpsample>();
    upsample->init(context, {redraw, _normals, depthReconstruction, _viewport, ffmpegED});
    
    // temportal upsampling, uses 'history buffer' and motion vectors
    compose = make_shared<trayraceCompose>();
    compose->init(context, {/*_viewport, */discontinuities, _motionVectors, upsample});
    
    
    
    // finish upscale
    
    switcher = make_shared<mush::switcherProcess>();
    auto temp = motionEdge->getEdgeSamples();
    auto temp2 = depthEdge->getEdgeSamples();
	//switcher->init(context, temp);
    
    _output = std::make_shared<mush::fixedExposureProcess>(0.0f);
    _output->init(context, upsample);

	_motion_reprojection = std::make_shared<motionReprojection>();
	_motion_reprojection->init(context, { _viewport, _motionVectors });
    
    // gui
    
    compose->setTagInGuiName("Full reconstruction");
    _guiBuffers.push_back(compose);
    upsample->setTagInGuiName("Spatially upsampled image");
    _guiBuffers.push_back(upsample);
    
    ffmpegED->setTagInGuiName("Encoded-Decoded h264");
    _guiBuffers.push_back(ffmpegED);
    ffmpegED_full->setTagInGuiName("Full H.264");
    _guiBuffers.push_back(ffmpegED_full);
    
    scaleImage->setTagInGuiName("Downscaled Image");
    //_guiBuffers.push_back(scaleImage);
    
    depthMult->setTagInGuiName("Scaled Depth");
    _guiBuffers.push_back(depthMult);
	//depthDelay->setTagInGuiName("Delayed Depth");
	//_guiBuffers.push_back(depthDelay);
    
    auto depths = depthEdge->getGuiBuffers();
    _guiBuffers.insert(_guiBuffers.end(), depths.begin(), depths.end());
    
    auto motions = motionEdge->getGuiBuffers();
    _guiBuffers.insert(_guiBuffers.end(), motions.begin(), motions.end());
    
    discontinuities->setTagInGuiName("Discontinuities");
    _guiBuffers.push_back(discontinuities);
    
    redraw->setTagInGuiName("Redrawn Segments");
    _guiBuffers.push_back(redraw);

	_motion_reprojection->setTagInGuiName("Motion Reproj Demo");
	_guiBuffers.push_back(_motion_reprojection);
    
    //redraw->null(); // DISABLED
    
    //switcher->setTagInGuiName("Switcher");
    //_guiBuffers.push_back(switcher);
    
	/*
    testsw = std::make_shared<mush::testSwscale>(avcodec_codec::x264, true);
    _viewport->addRepeat();
    testsw->init(context, _viewport);
    
    testsw->setTagInGuiName("Swscale Test (Clamp)");
    _guiBuffers.push_back(testsw);
    
    testsw2 = std::make_shared<mush::testSwscale>(avcodec_codec::x264, false);
    testsw2->init(context, _viewport);
    
    testsw2->setTagInGuiName("Swscale Test (FFmpeg)");
    _guiBuffers.push_back(testsw2);
    */
    // end gui

    /*
    _timer->add_metric(context, { _viewport, _viewport }, mush::metrics::psnr, "nop");
	*/
    _timer->add_metric(context, { _viewport, compose }, mush::metrics::psnr, "ST");
    _timer->add_metric(context, { _viewport, upsample }, mush::metrics::psnr, "Spatial");
    _timer->add_metric(context, { _viewport, ffmpegED_upscale }, mush::metrics::psnr, "Small");
    _timer->add_metric(context, { _viewport, ffmpegED_full }, mush::metrics::psnr, "Full");
    /*
    _timer->add_metric(context, { _viewport, _viewport }, mush::metrics::ssim, "nop");
	*/
    _timer->add_metric(context, { _viewport, compose }, mush::metrics::ssim, "ST");
    _timer->add_metric(context, { _viewport, upsample }, mush::metrics::ssim, "Spatial");
    _timer->add_metric(context, { _viewport, ffmpegED_upscale }, mush::metrics::ssim, "Small");
    _timer->add_metric(context, { _viewport, ffmpegED_full }, mush::metrics::ssim, "Full");

	// logpsnr

	_timer->add_metric(context, { _viewport, compose }, mush::metrics::log_psnr, "ST");
	_timer->add_metric(context, { _viewport, upsample }, mush::metrics::log_psnr, "Spatial");
	_timer->add_metric(context, { _viewport, ffmpegED_upscale }, mush::metrics::log_psnr, "Small");
	_timer->add_metric(context, { _viewport, ffmpegED_full }, mush::metrics::log_psnr, "Full");

	_timer->register_metric(ffmpegED->get_encoder(), "Packet Size (small)");
	_timer->register_metric(ffmpegED_full->get_encoder(), "Packet Size (full)");

	auto metric_gui = _timer->get_metric_gui();
	_guiBuffers.insert(_guiBuffers.end(), metric_gui.begin(), metric_gui.end());
};

void trayraceProcessor::process() {

	steppers[0]->process();
    /*
    testsw->process();
    testsw2->process();
    */
    depthMult->process();
    depthDelay->process();
    //        steppers[1]->process();
    discontinuities->process();
    
    scaleImage->process();
    temp->process();
    
    {
        //redraw->getRedrawMap().outLock();
        //redraw->getRedrawMap().outUnlock();
    }
    
    depthEdge->process();
    motionEdge->process();
    
	ffmpegED_upscale->process();

    upsample->process();
    
    redraw->process();
    
    compose->process();
    
    _output->process();

	_motion_reprojection->process();
    
    _timer->process_metrics();

	_timer->metric(ffmpegED->get_encoder());
	_timer->metric(ffmpegED_full->get_encoder());
    
    _timer->print_metered_report();
    
    //switcher->process();
    
    
    for (auto img : _nulls) {
        img->outUnlock(); // removes the need for nullers; hacky hacky hack hack
    }

}

const std::vector<std::string> trayraceProcessor::listKernels() {
    std::vector<std::string> kernels;
    return kernels;
}

const std::vector<std::shared_ptr<mush::ringBuffer>> trayraceProcessor::getBuffers() const {
    return {_output};
}

std::vector<std::shared_ptr<mush::guiAccessible>> trayraceProcessor::getGuiBuffers() {
    const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
    _guiBuffers.clear();
    return buffs;
}

std::vector<std::shared_ptr<mush::frameStepper>> trayraceProcessor::getFrameSteppers() const {
    return steppers;
}

void trayraceProcessor::go() {
    SetThreadName("Remote Render");
    
    std::thread ffmpeg(&mush::ffmpegEncodeDecode::go, ffmpegED);
    std::thread ffmpeg2(&mush::ffmpegEncodeDecode::go, ffmpegED_full);
    
    while (/*!quitHandler->getQuit() && */_viewport->good()) {
        process();
    }
    
    _timer->print_final_report();
    /*
    psnr->finalReport();
    psnr2->finalReport();
    psnr3->finalReport();
    psnr4->finalReport();
    
    ssim->finalReport();
    ssim2->finalReport();
    ssim3->finalReport();
    ssim4->finalReport();
    */
    
    if (depthEdge != nullptr) {
        depthEdge->release();
    }
    
    if (discontinuities != nullptr) {
        discontinuities->release();
    }
    
    if (motionEdge != nullptr) {
        motionEdge->release();
    }
    
    if (scaleImage != nullptr) {
        scaleImage->release();
    }
    
    if (redraw != nullptr) {
        redraw->release();
    }
    
    if (compose != nullptr) {
        compose->release();
    }
    
    if (switcher != nullptr) {
        switcher->release();
    }

	if (temp != nullptr) {
		temp->release();
	}

	if (scaleImage != nullptr) {
		scaleImage->release();
	}
	
	if (ffmpegED != nullptr) {
		ffmpegED->release();
	}
	
	if (ffmpegED_full != nullptr) {
		ffmpegED_full->release();
	}

	if (ffmpegED != nullptr) {
		ffmpegED->kill();
	}

	if (ffmpegED_full != nullptr) {
		ffmpegED_full->kill();
    }

	if (ffmpegED_upscale != nullptr) {
		ffmpegED_upscale->release();
	}
    
    if (_output != nullptr) {
        _output->release();
    }

	if (_motion_reprojection != nullptr) {
		_motion_reprojection->release();
	}
    /*
    if (testsw != nullptr) {
        testsw->release();
    }
    
    if (testsw2 != nullptr) {
        testsw2->release();
    }
    */
	try {
		if (ffmpeg.joinable()) {
			ffmpeg.join();
		}
		if (ffmpeg2.joinable()) {
			ffmpeg2.join();
		}
	} catch (std::exception& e) {
		putLog(e.what());
	}
}

std::shared_ptr<trayraceRedraw> trayraceProcessor::getRedraw() {
    return redraw;
}

