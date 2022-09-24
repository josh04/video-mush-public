//
//  hdrControl.cpp
//  video-mush
//
//  Created by Visualisation on 08/03/2014.
//  Copyright (c) 2014. All rights reserved.
//
#include <boost/asio.hpp>
#include "ConfigStruct.hpp"
#include "dll.hpp"

#include <Mush Core/gui.hpp>
#include <Mush Core/mushLog.hpp>
#include <azure/events.hpp>
#include <azure/eventtypes.hpp>
#include <azure/chrono.hpp>

#include <Mush Core/opencl.hpp>
#include <Mush Core/ringBuffer.hpp>
#include <Mush Core/tagInGui.hpp>
#include "hdrFileBuffer.hpp"
#include "hdrProcessor.hpp"
#include "exrEncoder.hpp"
#include "gifEncoder.hpp"
#include "screenStream.hpp"
#include "hdrTonemap.hpp"
#include "rgb16bitInput.hpp"
#include "flare10capture.hpp"
#include "mushPreprocessor.hpp"
#include "testCardInput.hpp"
#include "hdrJpegInput.hpp"
#ifndef __APPLE__
//#include "nvEncStream.hpp"
#endif

#include "exportInput.hpp"
#include "hdrEXRInputBuffer.hpp"
#include "udpFrameBuffer.hpp"

#include "videoFileInput.hpp"

#include "metadataEncoder.hpp"
#include "avcodecEncoder.hpp"
#include "libavformatOutput.hpp"
#include "nullOutput.hpp"
#include "averagesProcess.hpp"
#include "nullEncoder.hpp"

#include "mergeProcessor.hpp"

#include "canonInput.hpp"
#include "yuv10bitInput.hpp"


#include "yuvRawEncoder.hpp"
#include "yuvDepthClampProcess.hpp"
#include "fixedBitDepthProcessor.hpp"


#include "hdrControl.hpp"

#include "waveformProcessor.hpp"
#include "sandProcessor.hpp"

#include "singleEXRInput.hpp"
#include "mlvRawInput.hpp"

#include <Mush Core/quitEventHandler.hpp>
#include <Mush Core/stepperEventHandler.hpp>
#include <Mush Core/fakeWindow.hpp>
#include <Mush Core/mushWindowInterface.hpp>
#include "debayerProcessor.hpp"
#include "demoMode.hpp"

/*
 
 hdrControl.cpp
 
 This file contails the overall logic of the program. It's functions
 launch threads containing waiting ring buffers which take frames in
 and give (or write) frames out. There are n input buffers from
 input through 'compression', which is the main actor of the
 program. There are then n output buffers which organise the edited
 data to output. In practise, this means there runs the file read,
 a preprocessor which turns disparate data into cl image buffers,
 a compression method or similar, and then a video encoder
 which provdes output.
 
 */

hdrControl::hdrControl(mush::config config) :
	config(config) {
    destroyed = false;
}

void hdrControl::internalCreateContext() {
	if (!config.gui.show_gui) {
		context = make_shared<mush::opencl>(config.resourceDir, config.openclCPU);
		context->init();
    } else {
		_fake_window = std::make_shared<mush::gui::fake_window>();
        //tagGui = make_shared<tagInGui>();
        //tagGui->createGLContext(config.gui.sim2preview, config.gui.fullscreen, config.resourceDir);
		//context = tagGui->createInteropContext(config.resourceDir, config.openclCPU);
        
        auto scarlet_interface = std::make_shared<mush::gui::window_interface>(config.resourceDir);
        scarlet::PlatformInterface::SetPlatformInterface(scarlet_interface);
        
		context = std::make_shared<mush::opencl>(config.resourceDir, config.openclCPU);
		context->init(true);
	}
	putLog("-- Finished Making Contexts");
}

void hdrControl::precreateGLContext() {
    if (config.gui.show_gui) {
        tagGui = make_shared<tagInGui>();
        tagGui->createGLContext(config.gui.sim2preview, config.gui.fullscreen, config.resourceDir);
        putLog("-- Precreated GL Context");
    } else {
        putLog("-- Asked to precreate GL but GUI is disabled.");
    }
}

void hdrControl::externalCreateContext(cl::Context * ctx) {
    auto ctx2 = make_shared<mush::opencl>(config.resourceDir, config.openclCPU);
    
    ctx2->init(ctx);
    
    if (config.gui.show_gui) {
        tagGui->setContext(ctx2);
		tagGui->postContextSetup();
    }
    
    context = ctx2;
    putLog("-- Finished Applying External CL Context");
}

std::shared_ptr<mush::ringBuffer> hdrControl::addInput() {
    std::shared_ptr<mush::frameGrabber> inputBuffer = nullptr;
    
    boost::filesystem::path inP = config.inputConfig.inputPath;
    
    if (inP.extension() == ".mlv" || inP.extension() == ".MLV") {
        config.inputConfig.inputEngine = mush::inputEngine::mlvRawInput;
    }
    
    std::string tmp = std::string(config.inputConfig.resourceDir) + std::string(config.inputConfig.testCardPath);
    
    switch(config.inputConfig.inputEngine) {
        case mush::inputEngine::folderInput:
            inputBuffer = make_shared<hdrFileBuffer>();
            std::dynamic_pointer_cast<hdrFileBuffer>(inputBuffer)->moveToFrame(config.inputConfig.frame_skip);
            break;
        case mush::inputEngine::videoInput:
            inputBuffer = make_shared<VideoFileInput>(true);
        break;
        case mush::inputEngine::rgb16bitInput:
            inputBuffer = make_shared<RGB16bitInput>();
            break;
        case mush::inputEngine::flareInput:
        case mush::inputEngine::flare10Input:
            flare10s.push_back(make_shared<hdrFlare10>());
            inputBuffer = std::dynamic_pointer_cast<mush::frameGrabber>(flare10s.back());
        break;
        case mush::inputEngine::fastEXRInput:
            inputBuffer = make_shared<hdrEXRInputBuffer>();
        break;
        case mush::inputEngine::udpInput:
#ifndef _WIN32
            inputBuffer = make_shared<udpFrameBuffer>();
#endif
        break;
        case mush::inputEngine::testCardInput:
            config.inputConfig.inputPath = tmp.c_str();
            inputBuffer = make_shared<mush::singleEXRInput>();
            break;
        case mush::inputEngine::jpegInput:
            inputBuffer = make_shared<hdrJpegInput>();
        break;
        case mush::inputEngine::externalInput:
            inputBuffer = make_shared<exportInput>();
            break;
		case mush::inputEngine::canonInput:
			config.inputConfig.gammacorrect = 2.2f;
            inputBuffer = make_shared<hdrCanonInput>();
			break;
		case mush::inputEngine::yuv10bitInput:
			inputBuffer = make_shared<YUV10bitInput>();
			break;
        case mush::inputEngine::singleEXRInput:
            inputBuffer = make_shared<mush::singleEXRInput>();
            break;
        case mush::inputEngine::mlvRawInput:
		{
			auto buf = make_shared<mush::mlvRawInput>(config.inputConfig.blackPoint, 0, config.inputConfig.frame_skip);
			inputBuffer = buf;
			addEventHandler(buf);
		}
			break;
        default:
        case mush::inputEngine::noInput:
            inputBuffer = nullptr;
            return nullptr;
    }
	inputBuffer->setConfig(config.inputConfig);
	inputBuffer->init(context, {});
    //inputBuffer->getDetails(config.inputConfig);
    
    //inThreads.push_back(new std::thread(&mush::frameGrabber::startThread, inputBuffer));
    
    inputBuffers.push_back(inputBuffer);
    
    auto pre = preprocess(inputBuffer);
	preprocessBuffers.push_back(pre);
	return pre;
}

std::shared_ptr<mush::ringBuffer> hdrControl::preprocess(std::shared_ptr<mush::frameGrabber> inputBuffer) {
    std::shared_ptr<mush::mushPreprocessor> preprocessBuffer = make_shared<mush::mushPreprocessor>(config.inputConfig);
    preprocessBuffer->init(context, inputBuffer);
    
    preprocessThreads.push_back(new std::thread(&mush::mushPreprocessor::go, preprocessBuffer));
    
	guiBuffers.push_back(preprocessBuffer);
    
    if (config.demoMode) {
        demoMode = std::make_shared<mush::demoMode>();
        demoMode->init(context, preprocessBuffer);
        preprocessThreads.push_back(new std::thread(&mush::imageProcessor::startThread, demoMode));
        auto gets = demoMode->getBuffers();
        
        auto buffers = demoMode->getGuiBuffers();
        guiBuffers.insert(guiBuffers.end(), buffers.begin(), buffers.end());
        auto stepp = demoMode->getFrameSteppers();
        steppers.insert(steppers.end(), stepp.begin(), stepp.end());
        
        return gets.begin()[0];
    }
    
    if (config.inputConfig.inputEngine == mush::inputEngine::mlvRawInput) {
        
        auto mlv = std::dynamic_pointer_cast<mush::mlvRawInput>(inputBuffer);
        mlvBuffer = std::make_shared<mush::debayerProcessor>(config.inputConfig.whitePoint, mlv->get_camera_type(), config.inputConfig.dualISO, config.inputConfig.dual_iso_comp_factor, config.inputConfig.raw_clamp);
        mlvBuffer->init(context, preprocessBuffer);
        preprocessThreads.push_back(new std::thread(&mush::imageProcessor::startThread, mlvBuffer));
        auto gets = mlvBuffer->getBuffers();
        
        auto buffers = mlvBuffer->getGuiBuffers();
        guiBuffers.insert(guiBuffers.end(), buffers.begin(), buffers.end());
		guiBuffers[guiBuffers.size() - 5]->rocketGUI("scrub.rml");
        return gets.begin()[0];
    } else {
		preprocessBuffer->rocketGUI("scrub.rml");
        addEventHandler(preprocessBuffer);
        return preprocessBuffer;
    }
}

std::shared_ptr<mush::imageProcess> hdrControl::merge(std::initializer_list<std::shared_ptr<mush::ringBuffer>> inBuffs) {
    if (config.inputConfig.exposures == 2 || config.inputConfig.exposures == 3) {
        mergeBuffer = make_shared<mergeProcessor>(config.isoArray, config.inputConfig.gammacorrect, config.inputConfig.exposures); // WAS SHOW_GUI
        
        mergeBuffer->init(context, {inBuffs.begin()[0]});
        
        auto buffers = mergeBuffer->getGuiBuffers();
        guiBuffers.insert(guiBuffers.end(), buffers.begin(), buffers.end());
        
        mergeThread = new std::thread(&mush::imageProcessor::startThread, mergeBuffer);
        
        return std::dynamic_pointer_cast<mush::imageProcess>(mergeBuffer->getBuffers().begin()[0]);
    }
    return nullptr;
}

std::vector<std::shared_ptr<mush::ringBuffer>> hdrControl::process_init(std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffs, std::shared_ptr<mush::imageProcessor> hg) {
    
	videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(hg));
    encoder = hg;
    if (encoder != nullptr) {
        encoder->init(context, buffs);
        auto temp = encoder->getFrameSteppers();
        steppers.insert(steppers.end(), temp.begin(), temp.end());
        auto buffers = encoder->getGuiBuffers();
        guiBuffers.insert(guiBuffers.begin(), buffers.begin(), buffers.end());
        
        auto eventables = encoder->getEventables();
        
        for(auto& e : eventables) {
            videoMushAddEventHandler(e);
        }
        
        
        //processThread = new std::thread(&mush::imageProcessor::startThread, encoder);
        return encoder->getBuffers();
    } else {
        return {};
    }
    
}

void hdrControl::process_launch_thread() {
    for (auto& inputBuffer : inputBuffers) {
        inThreads.push_back(new std::thread(&mush::frameGrabber::startThread, inputBuffer));
    }
    
	if (encoder != nullptr) {
		processThread = new std::thread(&mush::imageProcessor::startThread, encoder);
	}
}

void hdrControl::encode(std::vector<std::shared_ptr<mush::ringBuffer>> outBuffers) {
	for (auto out : outBuffers) {
        auto ptr = std::dynamic_pointer_cast<mush::guiAccessible>(out);
        if (ptr != nullptr) {
            ptr->setTagInGuiRecording(true);
        }
	}
	unsigned int i = 1;
    
    avcodec_codec c;
    
    switch (config.outputConfig.encodeEngine) {
        case mush::encodeEngine::x264:
            c = avcodec_codec::x264;
            break;
        case mush::encodeEngine::x265:
            c = avcodec_codec::x265;
            break;
        case mush::encodeEngine::vpx:
            c = avcodec_codec::vpx;
            break;
        case mush::encodeEngine::prores:
            c = avcodec_codec::prores;
            break;
    }
    
    for (auto out : outBuffers) {
		if (out == nullptr) {
			continue;
		}
        if (std::dynamic_pointer_cast<averagesProcess>(out)) {
            
            encodeEngines.push_back(make_shared<metadataEncoder>());
            encodeEngines.back()->init(context, out, config.outputConfig);
            
        } else if (std::dynamic_pointer_cast<mush::imageBuffer>(out)) {
            
            switch (config.outputConfig.encodeEngine) {
                case mush::encodeEngine::x264:
                case mush::encodeEngine::x265:
                case mush::encodeEngine::vpx:
                case mush::encodeEngine::prores:
                    encodeEngines.push_back(make_shared<avcodecEncoder>(c, config.outputConfig.use_auto_decode));
                    break;
                case mush::encodeEngine::exr:
                    encodeEngines.push_back(make_shared<exrEncoder>(i));
                    break;
                case mush::encodeEngine::gif:
                    encodeEngines.push_back(make_shared<gifEncoder>(i));
                    break;
                case mush::encodeEngine::yuvRaw:
                {
                    //int i = 1;
                    encodeEngines.push_back(make_shared<mush::yuvRawEncoder>(i));
                }
                    break;
                case mush::encodeEngine::none:
                    encodeEngines.push_back(make_shared<nullEncoder>());
                    break;
            }
            
            encodeEngines.back()->init(context, out, config.outputConfig);
            
        } else if (std::dynamic_pointer_cast<avcodecEncoder>(out)) {
            
            encodeEngines.push_back(std::dynamic_pointer_cast<avcodecEncoder>(out));
            
        } else {
            putLog("Warning: process has incompatible outputs");
            encodeEngines.push_back(make_shared<nullEncoder>());
            encodeEngines.back()->init(context, out, config.outputConfig);
            
        }
        encodeThreads.push_back(new std::thread(&encoderEngine::startThread, encodeEngines.back()));
		++i;
    }
    
    for (auto encoder : encodeEngines) {
        auto buffers = encoder->getGuiBuffers();
        guiBuffers.insert(guiBuffers.end(), buffers.begin(), buffers.end());
    }
}

void hdrControl::output(std::vector<std::shared_ptr<mush::ringBuffer>> outBuffers) {
    encode(outBuffers);
    
    switch(config.outputConfig.outputEngine) {
//        case mush::outputEngine::screenOutput:
//        encodeScreen();
//        break;
        case mush::outputEngine::libavformatOutput:
            out = std::dynamic_pointer_cast<outputEngine>(std::make_shared<libavformatOutput>());
            out->init(encodeEngines, config.outputConfig);
            outThread = new boost::thread(boost::bind(&outputEngine::startThread, out));
            break;
        case mush::outputEngine::noOutput:
            out = std::dynamic_pointer_cast<outputEngine>(std::make_shared<nullOutput>());
            out->init(encodeEngines, config.outputConfig);
            outThread = new boost::thread(boost::bind(&outputEngine::startThread, out));
    }
}

void hdrControl::gui() {
    if (config.gui.show_gui) {
		tagGui = make_shared<tagInGui>();
        tagGui->createGLContext(config.gui.sim2preview, config.gui.fullscreen, config.resourceDir);
		tagGui->setContext(context);
		tagGui->postContextSetup();

        tagGui->init(context, guiBuffers, config.gui.exrDir, config.gui.subScreenRows);
        guiInitialised = true;
        for (auto ev : handlers) {
            tagGui->gui->addEventHandler(ev);
        }
        handlers.clear();
        quitter = std::make_shared<mush::quitEventHandler>();
        stepperHandler = std::make_shared<mush::stepperEventHandler>(steppers);
        tagGui->gui->addEventHandler(quitter);
        tagGui->gui->addEventHandler(stepperHandler);
    }
    
    if (config.inputConfig.inputEngine == mush::inputEngine::flare10Input) {
        if (auto merger = std::dynamic_pointer_cast<mergeProcessor>(mergeBuffer)) {
            for (int i = 0; i < flare10s.size(); ++i) {
                merger->setIsos(1.0, flare10s[i]->getMiddleFrameIso(), flare10s[i]->getTopFrameIso());
            }
        }
    }
    if (config.gui.show_gui) {
        if (outThread != nullptr) {
            
            while (!outThread->timed_join(boost::posix_time::millisec(2))) {
                /*
                int i = tagGui->gui->events();

                if (i > 0) {
                    if (config.inputConfig.inputEngine == mush::inputEngine::flare10Input) {
                        flareEvents(i);
                    }
                    
                }
				*/
                if (quitter->getQuit()) {
					tagGui->gui->removeEventHandler(quitter);
                    quitter = nullptr;
                    
                    azure::Events::Push(std::unique_ptr<azure::QuitEvent>(new azure::QuitEvent()));
                    tagGui->update();

					goto time_to_go;
                }
                
                tagGui->update();
                
            }
            azure::Events::Push(std::unique_ptr<azure::QuitEvent>(new azure::QuitEvent()));
            tagGui->update();
        }
    }

	if (outThread != nullptr) {
		if (outThread->joinable()) {
			outThread->join();
		}
	}

time_to_go:

	if (config.gui.show_gui) {
		if (quitter != nullptr) {
			tagGui->gui->removeEventHandler(quitter);
		}
		if (stepperHandler != nullptr) {
			tagGui->gui->removeEventHandler(stepperHandler);
		}
	}

	videoMushRemoveEventHandler(std::dynamic_pointer_cast<azure::Eventable>(encoder));
}

void hdrControl::destroy() {
    if (destroyed) {
        return;
    }
    destroyed = true;


    flare10s.clear();
    
    if (stepperHandler != nullptr) {
        stepperHandler->destroy();
    }
    
    if (demoMode != nullptr) {
        std::dynamic_pointer_cast<mush::demoMode>(demoMode)->release();
    }
    
    for (int i = 0; i < inputBuffers.size(); ++i) {
		auto buf = std::dynamic_pointer_cast<azure::Eventable>(inputBuffers[i]);
		if (buf != nullptr) {
			removeEventHandler(buf);
		}
        if (inputBuffers[i] != nullptr) {
            inputBuffers[i]->destroy();
        }
    }

	for (auto inT : inThreads) {
		if (inT != nullptr) {
			if (inT->joinable()) {
				inT->join();
			}
		}
	}

	for (int i = 0; i < preprocessBuffers.size(); ++i) {
		if (preprocessBuffers[i] != nullptr) {
			preprocessBuffers[i]->destroy();
		}
	}

	preprocessBuffers.clear();

    for (auto inT : preprocessThreads) {
        if (inT != nullptr) {
            if (inT->joinable()) {
                inT->join();
            }
        }
    }

	inputBuffers.clear();
    
    if (mergeThread != nullptr) {
        if (mergeThread->joinable()) {
            mergeThread->join();
        }
    }

	mergeBuffer.reset();

	if (outThread != nullptr) {
		if (outThread->joinable()) {
			outThread->join();
		}
	}

	encoder.reset();

	guiBuffers.clear();
	encodeEngines.clear();

	out.reset();


    if (processThread != nullptr) {
        if (processThread->joinable()) {
            processThread->join();
        }
    }

	mlvBuffer.reset();
	demoMode.reset();

	steppers.clear();
	quitter = nullptr;
	stepperHandler = nullptr;
	handlers.clear();
    
	if (tagGui != nullptr) {
		tagGui->drop_gl_cl_interop();
	}
    context.reset();

	tagGui.reset();
	_fake_window.reset();
}

/*
void hdrControl::encodeScreen() {
	screen = make_shared<screenStream>(outBuffer, 2560, 720);
	screen->init(config.resourceDir, config.outputConfig.width, config.outputConfig.height);
	outThread = new boost::thread(boost::bind(&screenStream::go, screen));
}*/

void hdrControl::flareEvents(int i) {
	/*
    for (int j = 0; j < flare10s.size(); ++j) {
        bool changed = false;
        switch (i) {
        case _EV_AWB:
            flare10s[j]->setAWB();
            break;
        case _EV_ISO_A_UP:
            flare10s[j]->isoUp(0);
            changed = true;
            break;
        case _EV_ISO_A_DOWN:
            flare10s[j]->isoDown(0);
            changed = true;
            break;
        case _EV_ISO_B_UP:
            flare10s[j]->isoUp(1);
            changed = true;
            break;
        case _EV_ISO_B_DOWN:
            flare10s[j]->isoDown(1);
            changed = true;
            break;
        case _EV_ISO_C_UP:
            flare10s[j]->isoUp(2);
            changed = true;
            break;
        case _EV_ISO_C_DOWN:
            flare10s[j]->isoDown(2);
            changed = true;
            break;
        }
        if (changed) {
            if (auto merger = std::dynamic_pointer_cast<mergeProcessor>(mergeBuffer)) {
                merger->setIsos(1.0, flare10s[j]->getMiddleFrameIso(), flare10s[j]->getTopFrameIso());
            }
        }
    }*/
}

std::shared_ptr<mush::ringBuffer> hdrControl::getLastInput() {
     return inputBuffers.back();
}

void hdrControl::addEventHandler(std::shared_ptr<azure::Eventable> ev) {
    if (guiInitialised) {
        tagGui->gui->addEventHandler(ev);
    } else {
        handlers.push_back(ev);
    }
}

void hdrControl::removeEventHandler(std::shared_ptr<azure::Eventable> ev) {
    if (tagGui != nullptr) {
        tagGui->gui->removeEventHandler(ev);
    }
}

void hdrControl::rekernel() {
    context->rekernel();
}
