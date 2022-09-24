#ifdef _WIN32
#include <boost/asio.hpp>
#include "windows.h"

#ifdef VIDEO_MUSH_EXPORTS
#define VIDEOMUSH_EXPORTS __declspec(dllexport) 
#else
#define VIDEOMUSH_EXPORTS __declspec(dllimport) 
#endif
#else
#define VIDEOMUSH_EXPORTS 
#endif


#include <iostream>
#include <atomic>

#include <queue>
#include <mutex>
#include <condition_variable>


#include <Mush Core/SetThreadName.hpp>

#include "ConfigStruct.hpp"
#include "hdrControl.hpp"
#include "exports.hpp"
#include "exportInput.hpp"

#include "hdrProcessor.hpp"
#include "nullProcessor.hpp"
#include "sandProcessor.hpp"
#include "waveformProcessor.hpp"
#include "genericProcessor.hpp"
#include "slicProcessor.hpp"
#include "fixedBitDepthProcessor.hpp"
#include "nuHDRProcessor.hpp"
#include "debayerProcessor.hpp"
#include "remote-render/trayraceProcessor.hpp"
#ifdef __APPLE__
#include <RadeonLibrary/radeonProcessor.hpp>
#include <parFramework/parProcessor.hpp>
#else
#include <App/Mush/radeonProcessor.hpp>
#include <PARtner/parProcessor.hpp>
#endif
#include "oculusProcessor.hpp"
#include "motionExplorer.hpp"
#include "metricProcessor.hpp"


#ifdef _WIN32 
#define snprintf sprintf_s
#endif

extern "C" {

std::shared_ptr<hdrControl> control = nullptr;

std::vector<std::shared_ptr<mush::ringBuffer>> _queued_outputs;

VIDEOMUSH_EXPORTS void videoMushRunAll(mush::config * config) {
    SetThreadName("mushControl");
    
	if (config->catch_exceptions) {
		try {
			videoMushInit(config);

			std::shared_ptr<mush::ringBuffer> input = nullptr;
			if (config->inputConfig.exposures > 1) {
				input = control->merge({ control->addInput() });
			} else {
				input = control->addInput();
			}

			videoMushCreateProcessor(config, { input });
		} catch (azure::exception::GL& e) {
			putLog(e.what());
			control->destroy();
			control.reset();
		} catch (std::runtime_error& e) {
			putLog(e.what());
			control->destroy();
			control.reset();
		} catch (std::exception& e) {
			putLog(e.what());
			putLog("Fatal Error; Check input values");
			control->destroy();
			control.reset();
		}
	} else {
		videoMushInit(config);

		std::shared_ptr<mush::ringBuffer> input = nullptr;
		if (config->inputConfig.exposures > 1) {
			input = control->merge({ control->addInput() });
		} else {
			input = control->addInput();
		}

		videoMushCreateProcessor(config, { input });
	}
//	_CrtDumpMemoryLeaks();
}

VIDEOMUSH_EXPORTS void videoMushInit(mush::config * config, bool useOwnContext) {
    SetThreadName("MushMain");
    putLog("- Pre Init");
    auto ptr = shared_ptr<hdrControl>(new hdrControl(*config), [](hdrControl * control){ control->destroy(); delete control; });
    control.swap(ptr);
    if (!useOwnContext) {
        control->internalCreateContext();
    } else {
        control->precreateGLContext();
    }
    
    putLog("- Passed Init");
}

VIDEOMUSH_EXPORTS void videoMushCreateProcessor(mush::config * config, std::initializer_list<std::shared_ptr<mush::ringBuffer>> list) {
    if (config == nullptr) {
        return;
    }
	mush::processEngine& processEngine = config->processEngine;
		
	std::shared_ptr<mush::imageProcessor> new_processor = nullptr;
    switch(processEngine) {
        case mush::processEngine::tonemapCompress:
            new_processor = make_shared<tonemapEncoder>();
            break;
        default:
        case mush::processEngine::none:
            new_processor = make_shared<nullProcessor>();
            break;
        case mush::processEngine::slic:
            new_processor = make_shared<slicProcessor>(config->slicConfig);
            break;
        case mush::processEngine::waveform:
            new_processor = make_shared<waveformProcessor>(config->waveformConfig.waveformMode);
            break;
        case mush::processEngine::sand:
            new_processor = make_shared<mush::sandProcessor>(config->generatorConfig, config->inputConfig.inputWidth, config->inputConfig.inputHeight, config->resourceDir, config->parConfig, *config);

			if (config->generatorConfig.type == mush::generatorProcess::motionReprojection) {
                if (std::string(control->config.inputConfig.secondInputPath).length() > 0) {
                    control->config.inputConfig.inputPath = control->config.inputConfig.secondInputPath;
                    auto i3 = control->addInput();
                    list = { list.begin()[0], i3 };
                }
                if (std::string(control->config.inputConfig.thirdInputPath).length() > 0) {
                    control->config.inputConfig.inputPath = control->config.inputConfig.thirdInputPath;
                    auto i3 = control->addInput();
                    list = { list.begin()[0], list.begin()[1], i3 };
                }
                if (std::string(control->config.inputConfig.fourthInputPath).length() > 0) {
                    control->config.inputConfig.inputPath = control->config.inputConfig.fourthInputPath;
                    auto i3 = control->addInput();
                    list = { list.begin()[0], list.begin()[1], list.begin()[2], i3 };
                }
			}
        
        if (config->generatorConfig.type == mush::generatorProcess::motionGenerator) {
            if (std::string(control->config.inputConfig.secondInputPath).length() > 0) {
                control->config.inputConfig.inputPath = control->config.inputConfig.secondInputPath;
                auto i3 = control->addInput();
                list = { list.begin()[0], i3 };
            }
        
        }

			if (config->generatorConfig.type == mush::generatorProcess::oculusVideo) {
				if (std::string(control->config.inputConfig.secondInputPath).length() > 0) {
					control->config.inputConfig.inputPath = control->config.inputConfig.secondInputPath;
					auto i3 = control->addInput();
					list = { list.begin()[0], i3 };
				}
			}
            
            if (config->generatorConfig.type == mush::generatorProcess::anaglyph) {
                
                //if (std::string(control->config.metric_path).length() > 0) {
					control->config.inputConfig.inputPath = control->config.inputConfig.secondInputPath;
					auto i3 = control->addInput();
					list = { list.begin()[0], i3 };
                //}
            }
        
        if (config->generatorConfig.type == mush::generatorProcess::sbsPack) {
            
            //if (std::string(control->config.metric_path).length() > 0) {
            control->config.inputConfig.inputPath = control->config.inputConfig.secondInputPath;
            auto i3 = control->addInput();
            list = { list.begin()[0], i3 };
            //}
        }
			
			break;
        case mush::processEngine::fixedBitDepth:
            new_processor = make_shared<fixedBitDepthProcessor>(config->outputConfig.yuvMax, config->outputConfig.yuvBitDepth, config->outputConfig.pqLegacy, config->func, config->fbd_output);
            break;
        case mush::processEngine::nuHDR:
            new_processor = make_shared<nuHDRProcessor>(config->inputConfig.darken);
            break;
        case mush::processEngine::generic:
            new_processor = make_shared<mush::genericProcessor>(config->genericChoice);
            break;
		case mush::processEngine::trayrace:
		{
			std::string depthPath = std::string(config->inputConfig.inputPath) + std::string("/depth");
			std::string normalsPath = std::string(config->inputConfig.inputPath) + std::string("/normals");
			std::string motionPath = std::string(config->inputConfig.inputPath) + std::string("/motion");
			control->config.inputConfig.inputPath = depthPath.c_str();
			auto i1 = control->addInput(); 
			control->config.inputConfig.inputPath = normalsPath.c_str();
			auto i2 = control->addInput(); 
			control->config.inputConfig.inputPath = motionPath.c_str();
			auto i3 = control->addInput();
			new_processor = make_shared<trayraceProcessor>(nullptr);
			list = { list.begin()[0], i1, i2, i3 };
		}
            break;
        case mush::processEngine::par:
        {
            auto par = make_shared<parProcessor>(config->parConfig, config->catch_exceptions);
            new_processor = par;
        }
            break;
        case mush::processEngine::amd:
        {

            auto par = make_shared<radeonProcessor>(config->radeonConfig, config->catch_exceptions);
            new_processor = par;
        }
            break;
		case mush::processEngine::oculusDraw:
		{

			auto par = make_shared<mush::oculusProcessor>(config->oculusConfig.source, config->parConfig, config->radeonConfig);
			new_processor = par;
		}
		break;
		case mush::processEngine::motionExplorer:
			new_processor = make_shared<mush::motionExplorer>(config->parConfig, config->motionExplorer);
            if (std::string(control->config.inputConfig.secondInputPath).length() > 0) {
                control->config.inputConfig.inputPath = control->config.inputConfig.secondInputPath;
                auto i3 = control->addInput();
                list = { list.begin()[0], i3 };
            }
            if (std::string(control->config.inputConfig.thirdInputPath).length() > 0) {
                control->config.inputConfig.inputPath = control->config.inputConfig.thirdInputPath;
                auto i3 = control->addInput();
                list = { list.begin()[0], list.begin()[1], i3 };
            }
			break;
		case mush::processEngine::metrics:
            control->config.inputConfig.inputPath = control->config.inputConfig.secondInputPath;
            auto i3 = control->addInput();
            list = { list.begin()[0], i3 };

            new_processor = make_shared<mush::metricProcessor>();
            break;
    }
    
    if (new_processor == nullptr) {
        throw std::runtime_error("Failed: No processor created.");
    }
	

    //videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(new_processor));
	std::vector<std::shared_ptr<mush::ringBuffer>> processes = control->process_init(list, new_processor);
	putLog("-- Passed Processor Initialisation");

	new_processor = nullptr;

	videoMushResetOutputNameCount();
	_queued_outputs.clear();
	_queued_outputs.insert(_queued_outputs.end(), processes.begin(), processes.end());
}

VIDEOMUSH_EXPORTS void videoMushUseExistingProcessor(std::shared_ptr<mush::imageProcessor> processor, std::initializer_list<std::shared_ptr<mush::ringBuffer>> list) {

	//videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(new_processor));
	std::vector<std::shared_ptr<mush::ringBuffer>> processes = control->process_init(list, processor);
	putLog("-- Passed Processor Initialisation");

	videoMushResetOutputNameCount();
	_queued_outputs.clear();
	_queued_outputs.insert(_queued_outputs.end(), processes.begin(), processes.end());
}

    
VIDEOMUSH_EXPORTS void videoMushExecute(int ar_size, bool ** use_outputs) {

	if (control == nullptr) {
		throw std::runtime_error("Failed to execute.");
	}

	decltype(_queued_outputs) used_outputs;
	bool * _use_outputs = *use_outputs;
	for (int i = 0; i < ar_size; ++i) {
		if (_use_outputs[i] && _queued_outputs.size() > i) {
			used_outputs.push_back(_queued_outputs[i]);
		}
 	}
	control->output(used_outputs);

	control->process_launch_thread();

	putLog("-- Passed Output");

	putLog("-- Running GUI");
	control->gui();
	
	//videoMushRemoveEventHandler(std::dynamic_pointer_cast<azure::Eventable>(hg));

	putLog("-- Destroying");
	control->destroy();
}

VIDEOMUSH_EXPORTS void videoMushDestroy() {
	if (control != nullptr) {
		control->destroy();
		control = nullptr;
	}
}
    
    VIDEOMUSH_EXPORTS void videoMushAddEventHandler(std::shared_ptr<azure::Eventable> ev){
		if (control != nullptr && ev != nullptr) {
            control->addEventHandler(ev);
        }
    }
    
    VIDEOMUSH_EXPORTS void videoMushRemoveEventHandler(std::shared_ptr<azure::Eventable> ev){
        if (control != nullptr && ev != nullptr) {
            control->removeEventHandler(ev);
        }
    }


	VIDEOMUSH_EXPORTS uint32_t videoMushGetOutputCount() {
		return _queued_outputs.size();
	}

	size_t cnt = 0;

	VIDEOMUSH_EXPORTS bool videoMushGetOutputName(char * name, uint64_t size) {
		if (cnt < _queued_outputs.size()) {
			auto gui = std::dynamic_pointer_cast<mush::guiAccessible>(_queued_outputs[cnt]);
			if (gui != nullptr) {
				auto str = gui->getTagInGuiName();

				snprintf(name, size, "%s", str.c_str());

				//strncpy_s(name, str.size(), str.c_str(), size);
				cnt++;
				return true;
			}
		}
		return false;
	}
	
	VIDEOMUSH_EXPORTS void videoMushResetOutputNameCount() {
		cnt = 0;
	}
    
    VIDEOMUSH_EXPORTS void videoMushUpdateCLKernels() {
        if (control != nullptr) {
            control->rekernel();
        }
    }

}

VIDEOMUSH_EXPORTS std::shared_ptr<mush::opencl> videoMushGetContext() {
	return control->getContext();
}
