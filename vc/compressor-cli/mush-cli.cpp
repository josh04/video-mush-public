//
//  main.cpp
//  compressor
//
//  Created by Jonathan Hatchett on 12/07/2012.
//  Copyright (c) 2012. All rights reserved.
//

#define __CL_ENABLE_EXCEPTIONS

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif

#ifdef _WIN32
#include <tchar.h>
#include "getopt/getopt.h"
#else
#include <getopt.h>
#endif

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <atomic>

#include <boost/filesystem.hpp>

#include <thread>
#include <chrono>

#include <Mush Core/mushLog.hpp>
#include "exports.hpp"

//#include "CL/cl.hpp"
//#include "clerr.h"

const char * usage = " \
Usage:  compressor \n \
Mode:              [--flare10 Flare] [--camera Dual Canons] [--video ML video file(s)] [--folder multi filetype (not JPEG) (default)] \n \
                   [--rgb16 16-bit rgb file input] [--jpeg JPEG input] [--udp FPGA input] [--testcard] \n \
Filetype:          [--raw] [--pfm] [--exr] [--tiff] \n \
OpenCL:            [--cpu] [--gpu (default)] \n \
GUI:               [--gui show the gui] [--sim2-preview] \n \
Merge :            [-i|--isos x,x,x,x] [-e exposures] \n \
Output:            [-o filename] [-w|--width width] [-h|--height height] [-f|--fps fps] \n \
                   [-b|--bitrate bitrate] [-z|--zerolatency] \n \
                   [--screen screen output] \n \
                   [--exr-output exr output] \n \
				   [--no-encode disable output] \n \
                   [--tonemap tonemap output] \n \
                   [--vlc use vlc for output \n \
                   [-m|--mode vlc mode (file, hls, http, old)] \n \
Misc:              [-d adjust input exposure] [-g|--gamma perform gamma correction] \n \
Input:             foldername \n";


void runLog(std::atomic_int * over) {
	const uint64_t size = 4096;
	char output[size];
	while (!(*over)) {
		std::this_thread::sleep_for(std::chrono::microseconds(2));
		bool set = false;
		set = getLog(output, size);
		if (set) {
			std::cout << output << std::endl;
		}
	}
};

int main(int argc, const char * argv[]) {
	//	try {

	mush::config config;
	config.defaults();

	config.processEngine = mush::processEngine::none;
	config.outputConfig.outputEngine = mush::outputEngine::noOutput;
	config.outputConfig.encodeEngine = mush::encodeEngine::none;
	
	enum class arg {
		gui,
		sim2,
		opencl_cpu,
		test_card,
		frames,
		video,
		camera,
		no_input,
		input_transfer,
		resize_input,
		process,
		text,
		ffmpeg,
		encoder,
		output_transfer,
		fps,
		crf,
		resize_output,
		exr_output,

		merge_two,
		merge_three,
		merge_dualiso,

		spherical,
		model,
		stereo,
        
        camera_read_path,
        camera_write_path,

        second_input_path,
        camera_path_quit,
        camera_path_stereo_shift,
        
		debug
	};

	const struct option longOptions[] = {

		{ "gui", no_argument, NULL, (int)arg::gui },
		{ "sim2-preview", no_argument, NULL, (int)arg::sim2 },
		{ "opencl-cpu", no_argument, NULL, (int)arg::opencl_cpu },
		{ "testcard", no_argument, NULL, (int)arg::test_card },

		{ "frames", required_argument, NULL, (int)arg::frames },
		{ "video", required_argument, NULL, (int)arg::video },
		{ "camera", required_argument, NULL, (int)arg::camera },
		{ "no-input", no_argument, (int*)&config.inputConfig.inputEngine, (int)mush::inputEngine::noInput },

		{ "loop", no_argument, NULL, 'l' },
		{ "start-frame", required_argument, NULL, 's' },

		{ "merge-two", required_argument, NULL, (int)arg::merge_two },
		{ "merge-three", required_argument, NULL, (int)arg::merge_three },
		{ "merge-dualiso", required_argument, NULL, (int)arg::merge_dualiso },


		{ "exposure", required_argument, NULL, 'd' },

		{ "input-transfer", required_argument, NULL, (int)arg::input_transfer },
		{ "resize-input", required_argument, NULL, (int)arg::resize_input },

		{ "process", required_argument, NULL, (int)arg::process },

		{ "text", required_argument, NULL, (int)arg::text },

		{ "ffmpeg", required_argument, NULL, (int)arg::ffmpeg },
		{ "encoder", required_argument, NULL, (int)arg::encoder },
		{ "output-transfer", required_argument, NULL, (int)arg::output_transfer },
		{ "fps", required_argument, NULL, (int)arg::fps },
		{ "crf", required_argument, NULL, (int)arg::crf },
		{ "resize-output", required_argument, NULL, (int)arg::resize_output },

		{ "exr-output", required_argument, NULL, (int)arg::exr_output },
		{ "spherical", no_argument, NULL, (int)arg::spherical },
		{ "model", required_argument, NULL, (int)arg::model },
		{ "stereo", no_argument, NULL, (int)arg::stereo },
        
        { "camera_read_path", required_argument, NULL, (int)arg::camera_read_path },
        { "camera_write_path", required_argument, NULL, (int)arg::camera_write_path },
        
        { "second-input-path", required_argument, NULL, (int)arg::second_input_path },
        { "camera-path-quit", no_argument, NULL, (int)arg::camera_path_quit },
        { "camera-path-stereo-shift", no_argument, NULL, (int)arg::camera_path_stereo_shift},
        
        
        { "width", required_argument, NULL, 'w' },
        { "height", required_argument, NULL, 'h' },


		{ "debug", no_argument, NULL, (int)arg::debug },
		/*
		{ "pfm", no_argument, (int*) &config.inputConfig.filetype, (int)mush::filetype::pfmFiletype },
		{ "raw", no_argument, (int*)&config.inputConfig.filetype, (int)mush::filetype::rawFiletype },
		{ "exr", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::fastEXRInput },
		{ "tiff", no_argument, (int*)&config.inputConfig.filetype, (int)mush::filetype::mergeTiffFiletype },
		
		//			{"gpu", no_argument, (int*)&config.cltype, CL_DEVICE_TYPE_GPU},
		//			{"cpu", no_argument, &cltype, CL_DEVICE_TYPE_CPU},

		{ "stdin", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::stdinInput },
		{ "video", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::videoInput },
		{ "rgb16", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::rgb16bitInput },
		{ "flare", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::flareInput },
		{ "flare10", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::flare10Input },
		{ "jpeg", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::jpegInput },
		{ "udp", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::udpInput },
		{ "testcard", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::testCardInput },
        { "yuv10", no_argument, (int*)&config.inputEngine, (int)mush::inputEngine::yuv10bitInput },

		{ "isos", required_argument, NULL, 'i' },

		{ "no-process", no_argument, (int*)&eng, (int)mush::processEngine::none },
//		{ "ldr", no_argument, (int*)&config.processEngine, (int)mush::processEngine::ldrCompress },
		{ "tonemap", no_argument, (int*)&eng, (int)mush::processEngine::tonemapCompress },
		{ "waveform", no_argument, (int*)&eng, (int)mush::processEngine::waveform },
		{ "saliency", no_argument, (int*)&eng, (int)mush::processEngine::slic },

//		{ "screen", no_argument, (int*)&config.outputEngine, (int)mush::outputEngine::screenOutput },
		{ "exr-encode", no_argument, (int*)&config.outputConfig.encodeEngine, (int)mush::encodeEngine::exr },
		{ "ffmpeg", no_argument, (int*)&config.outputConfig.encodeEngine, (int)mush::encodeEngine::x264 },
		{ "nvenc", no_argument, (int*)&config.outputConfig.encodeEngine, (int)mush::encodeEngine::nvenc },
		{ "vlc", no_argument, (int*)&config.outputEngine, (int)mush::outputEngine::vlcOutput },
		{ "exr-output", no_argument, (int*)&config.processEngine, (int)mush::processEngine::exrCompress },

		{ "ffmpeg", no_argument, (int*)&config.outputEngine, (int)mush::outputEngine::ffmpegOutput },
		{ "x264", no_argument, (int*)&config.outputEngine, (int)mush::outputEngine::x264Output },
		{ "nvenc", no_argument, (int*)&config.outputEngine, (int)mush::outputEngine::nvEncOutput },

		{ "fps", required_argument, NULL, 'f' },

		{ "bitrate", required_argument, NULL, 'b' },
		{ "zerolatency", no_argument, NULL, 'z' },
		{ "preset", required_argument, NULL, 'x' },
		{ "profile", required_argument, NULL, '#' },
		{ "chroma-subsample", required_argument, NULL, '>' },

		{ "darken", required_argument, NULL, 'd' },
		{ "gamma", required_argument, NULL, 'g' },
		*/
		{ 0, 0, 0, 0 }
	};



	std::string isos = "1,4,8,32";
	std::string input_path = "";
	std::string text_input = "";
	std::string output_path = "";
	std::string model_path = "";
    std::string camera_read_path = "";
    std::string camera_write_path = "";
    std::string second_input_path = "";
	
	int c = 0;
	int optionIndex = 0;

	while (true) {
		if ((c = getopt_long(argc, (char* const*) argv, "lsd?", longOptions, &optionIndex)) == -1) { break; }
		switch (c) {

		case (int)arg::gui:
			config.gui.show_gui = true;
			break;
		case (int)arg::sim2:
			config.gui.sim2preview = true;
			break;
		case (int)arg::opencl_cpu:
			config.openclCPU = true;
			break;
		case (int)arg::test_card:
			config.inputConfig.inputEngine = mush::inputEngine::testCardInput;
			break;
		case (int)arg::frames:
			config.inputConfig.inputEngine = mush::inputEngine::folderInput;
			input_path = std::string(optarg);
			break;
		case (int)arg::video:
			config.inputConfig.inputEngine = mush::inputEngine::videoInput;
			input_path = std::string(optarg);
			break;
		case (int)arg::camera:
		{
			int cam = atoi(optarg);
			if (cam == 1) {
				config.inputConfig.inputEngine = mush::inputEngine::canonInput;
			} else {
				config.inputConfig.inputEngine = mush::inputEngine::flare10Input;
			}
		}
			break;
		case (int)arg::no_input:
			config.inputConfig.inputEngine = mush::inputEngine::noInput;
			break;
		case (int)arg::input_transfer:
			if (optarg == std::string("linear")) {
				config.inputConfig.func = mush::transfer::linear;
			}
			if (optarg == std::string("srgb")) {
				config.inputConfig.func = mush::transfer::srgb;
			}
			if (optarg == std::string("pq")) {
				config.inputConfig.func = mush::transfer::pq;
			}
			if (optarg == std::string("ptf4")) {
				config.inputConfig.func = mush::transfer::g8;
			}
			if (optarg == std::string("logc")) {
				config.inputConfig.func = mush::transfer::logc;
			}
			if (optarg == std::string("rec709")) {
				config.inputConfig.func = mush::transfer::rec709;
			}
			if (optarg == std::string("gamma18")) {
				config.inputConfig.func = mush::transfer::gamma;
				config.inputConfig.gammacorrect = 1.8f;
			}
			if (optarg == std::string("gamma22")) {
				config.inputConfig.func = mush::transfer::gamma;
				config.inputConfig.gammacorrect = 2.2f;
			}
			if (optarg == std::string("gamma24")) {
				config.inputConfig.func = mush::transfer::gamma;
				config.inputConfig.gammacorrect = 2.4f;
			}
			break;
		case (int)arg::resize_input:
            {
                config.inputConfig.resize = true;
                std::string input_resize(optarg);
                char x = '\0';
                sscanf(input_resize.c_str(), "%d%c%d", &config.inputConfig.resize_width, &x, &config.inputConfig.resize_height);
            }
			break;
		case (int)arg::process:
			if (std::string(optarg) == std::string("none")) {
				config.processEngine = mush::processEngine::none;
			}
			if (optarg == std::string("sand")) {
				config.processEngine = mush::processEngine::sand;
				config.generatorConfig.type = mush::generatorProcess::sand;
			}
			if (optarg == std::string("remote-render")) {
				config.processEngine = mush::processEngine::trayrace;
			}
			if (optarg == std::string("barcode")) {
				config.processEngine = mush::processEngine::generic;
				config.genericChoice = mush::genericProcess::barcode;
			}
			if (optarg == std::string("radeonrays")) {
				config.processEngine = mush::processEngine::amd;
			}
			if (optarg == std::string("360video")) {
				config.processEngine = mush::processEngine::sand;
				config.generatorConfig.type = mush::generatorProcess::sphere;
			}
			if (optarg == std::string("oculus-video")) {
				config.processEngine = mush::processEngine::sand;
				config.generatorConfig.type = mush::generatorProcess::oculusVideo;
			}
			if (optarg == std::string("oculus-draw")) {
				config.processEngine = mush::processEngine::sand;
				config.generatorConfig.type = mush::generatorProcess::oculusDraw;
			}
			if (optarg == std::string("slic")) {
				config.processEngine = mush::processEngine::slic;
			}
			if (optarg == std::string("waveform")) {
				config.processEngine = mush::processEngine::waveform;
			}
			if (optarg == std::string("bit-depth")) {
				config.processEngine = mush::processEngine::fixedBitDepth;
			}
			if (optarg == std::string("text")) {
				config.processEngine = mush::processEngine::sand;
				config.generatorConfig.type = mush::generatorProcess::text;
			}
			if (optarg == std::string("par")) {
				config.processEngine = mush::processEngine::par;
			}
			if (optarg == std::string("motion-explorer")) {
				config.processEngine = mush::processEngine::motionExplorer;
            }
            if (optarg == std::string("metrics")) {
                config.processEngine = mush::processEngine::metrics;
            }
			if (optarg == std::string("raster")) {
				config.processEngine = mush::processEngine::sand;
				config.generatorConfig.type = mush::generatorProcess::raster;
			}
			break;
		case (int)arg::text:
			text_input = std::string(optarg);
			break;
		case (int)arg::ffmpeg:
			config.outputConfig.outputEngine = mush::outputEngine::libavformatOutput;
			output_path = std::string(optarg);
			break;
		case (int)arg::encoder:
			if (optarg == std::string("x264")) {
				config.outputConfig.encodeEngine = mush::encodeEngine::x264;
			}
			if (optarg == std::string("x265")) {
				config.outputConfig.encodeEngine = mush::encodeEngine::x265;
			}
			if (optarg == std::string("vp9")) {
				config.outputConfig.encodeEngine = mush::encodeEngine::vpx;
			}
			if (optarg == std::string("prores")) {
				config.outputConfig.encodeEngine = mush::encodeEngine::prores;
			}
			break;
		case (int)arg::output_transfer:

			if (optarg == std::string("srgb")) {
				config.outputConfig.func = mush::transfer::srgb;
			}
			if (optarg == std::string("ptf4")) {
				config.outputConfig.func = mush::transfer::g8;
			}
			if (optarg == std::string("pq")) {
				config.outputConfig.func = mush::transfer::pq;
			}
			if (optarg == std::string("linear")) {
				config.outputConfig.func = mush::transfer::linear;
            }
            if (optarg == std::string("rec709")) {
                config.outputConfig.func = mush::transfer::rec709;
            }
			break;
		case (int)arg::fps:
			config.outputConfig.fps = atof(optarg);
			break;
		case (int)arg::crf:
			config.outputConfig.crf = atoi(optarg);
			break;
		case (int)arg::resize_output:
			config.outputConfig.overrideSize = true;
			//
			break;
		case (int)arg::exr_output:
			config.outputConfig.outputEngine = mush::outputEngine::noOutput;
			config.outputConfig.encodeEngine = mush::encodeEngine::exr;
			output_path = std::string(optarg);
			break;
		case (int)arg::merge_two:
			config.inputConfig.exposures = 2;
			isos = optarg;
			break;
		case (int)arg::merge_three:
			config.inputConfig.exposures = 3;
			isos = optarg;
			break;
		case (int)arg::merge_dualiso:
			config.inputConfig.dualISO = true;
			config.inputConfig.dual_iso_comp_factor = atof(optarg);
			break;

		case (int)arg::spherical:
			config.cameraConfig.equirectangular = true;
                config.motionExplorer.spherical = true;
			break;
		case (int)arg::model:
			model_path = std::string(optarg);
			break;
		case (int)arg::stereo:
			config.cameraConfig.stereo = true;
			break;
                
        case (int)arg::camera_read_path:
            config.cameraConfig.autocam = true;
            camera_read_path = std::string(optarg);
            break;
        case (int)arg::camera_write_path:
            camera_write_path = std::string(optarg);
            break;
                
            case (int)arg::second_input_path:
                second_input_path = std::string(optarg);
                config.inputConfig.secondInputPath = second_input_path.c_str();
                break;
            case (int)arg::camera_path_quit:
                config.cameraConfig.quit_at_camera_path_end = true;
                break;
            case (int)arg::camera_path_stereo_shift:
                config.motionExplorer.follow_stereo = true;
                break;

		case (int)arg::debug:
			config.catch_exceptions = false;
			break;

		case 'l':
			config.inputConfig.loopFrames = true;
			break;
		case 's':
			config.inputConfig.frame_skip = atoi(optarg);
			break;
		case 'd':
			config.inputConfig.darken = atoi(optarg);
            break;
                
            case 'w': config.inputConfig.inputWidth = atoi(optarg); break;
            case 'h': config.inputConfig.inputHeight = atoi(optarg); break;


			/*
		case 'n': config.inputConfig.noflip = true; break;
		case 'o': outputPath = optarg; break;
		case 'e': config.inputConfig.exposures = atoi(optarg); break;

		case 'i': isos = optarg; break;

		case 'd': config.inputConfig.darken = atof(optarg); break;
		case 'g': config.inputConfig.gammacorrect = atof(optarg); break;

		case 'w': config.inputConfig.inputWidth = atoi(optarg); break;
		case 'h': config.inputConfig.inputHeight = atoi(optarg); break;
		case 'f': config.outputConfig.fps = atof(optarg); break;

//		case 'b': config.bitrate = atoi(optarg); break;
		case 'z': config.inputConfig.inputBuffers = 2; config.outputConfig.outputBuffers = 1; config.outputConfig.zerolatency = true; break;
		case 'x':
			if (optarg == std::string("slower") || optarg == std::string("slow") || optarg == std::string("medium") || optarg == std::string("fast") || optarg == std::string("faster")) {
				config.outputConfig.h264Preset = optarg;
			} break;
		case '#':
			if (optarg == std::string("baseline") || optarg == std::string("main") || optarg == std::string("high") || optarg == std::string("high10") || optarg == std::string("high422") || optarg == std::string("high444")) {
				config.outputConfig.h264Profile = optarg;
			} break;

		case '>':
			config.chromaSubsample = mush::chromaSubsample::yuv420p;
			if (atoi(optarg) == 422) {
				config.chromaSubsample = mush::chromaSubsample::yuv422p;
			}
			if (atoi(optarg) == 444) {
				config.chromaSubsample = mush::chromaSubsample::yuv444p;
			}
		case 's': config.sim2preview = true; break;
		case 'u': config.show_gui = true; break;
		*/

		case '?': std::cout << usage; return EXIT_SUCCESS; break;
		}
	}
	

	if (config.outputConfig.encodeEngine != mush::encodeEngine::exr && config.outputConfig.encodeEngine != mush::encodeEngine::none) {
		config.outputConfig.outputEngine = mush::outputEngine::libavformatOutput;
	}

    config.cameraConfig.model_path = model_path.c_str();
    config.cameraConfig.load_path = camera_read_path.c_str();
    config.cameraConfig.save_path = camera_write_path.c_str();
	config.parConfig.camera_config = config.cameraConfig;
	config.parConfig.use_mush_model = true;
    
    std::cerr << "Input path: " << input_path << std::endl;
    
	if (input_path.size() > 0) {
		boost::filesystem::path boost_input_path(input_path);
		if (boost::filesystem::exists(boost_input_path)) {
            input_path = boost_input_path.string();
			config.inputConfig.inputPath = input_path.c_str();
		} else {
			std::cerr << "Input path does not exist: " << input_path << std::endl;
            exit(0);
		}
	}
//	if (config.inputEngine != mush::folderInput && config.inputEngine != mush::videoInput) {
//		config.x264Pass = mush::x264Pass::onePass;
//	}

	float iso;
	std::istringstream sstream(isos);
	char k;
	for (int i = 0; i < 4; ++i) {
		sstream >> iso;
		sstream >> k;
		config.isoArray[i] = iso;
		//			std::sort(isoArray.begin(), isoArray.end());
	}

	std::string parent = "";
	std::string name = "";
	if (output_path.size() > 0) {
		boost::filesystem::path boost_output_path(output_path);
		parent = boost_output_path.parent_path().string();
		name = boost_output_path.filename().string();

		if (!boost_output_path.has_extension()) {
			name += ".mp4";
		}

		if (parent.length() > 0) {
			config.outputConfig.outputPath = parent.c_str();
		}

		if (name.length() > 0) {
			config.outputConfig.outputName = name.c_str();
		}
	}
    
    config.resourceDir = "./";
    config.inputConfig.resourceDir = "./";

#ifdef __APPLE__
    NSString * frDir = [NSString stringWithFormat:@"%@", @"Video Mush.framework"];
	NSString * resDir = [[[NSBundle bundleWithPath:frDir] resourcePath] stringByAppendingString:@"/"];
	config.resourceDir = [resDir UTF8String];
    config.inputConfig.resourceDir = [resDir UTF8String];
#endif

	const uint64_t size = 4096;
	char output[size];
	std::atomic<int> over(0);
	
	std::thread logThread(&runLog, &over);
	if (config.catch_exceptions) {
		try {
			videoMushRunAll(&config);

			auto output_count = videoMushGetOutputCount();
			bool * use_outputs = new bool[output_count];
			putLog("Outputs:");
			for (int i = 0; i < output_count; i++)
			{
				use_outputs[i] = true;
				char * name = new char[4096];
				videoMushGetOutputName(name, 4096);
				putLog(std::string(name));
				delete name;
			}
			videoMushExecute(output_count, &use_outputs);
			delete use_outputs;
		} catch (std::runtime_error& e) {
			putLog(e.what());
		} catch (std::exception& e) {
			putLog(e.what());
		}
	} else {
		videoMushRunAll(&config);

		auto output_count = videoMushGetOutputCount();
		bool * use_outputs = new bool[output_count];
		putLog("Outputs:");
		for (int i = 0; i < output_count; i++)
		{
			use_outputs[i] = true;
			char * name = new char[4096];
			videoMushGetOutputName(name, 4096);
			putLog(std::string(name));
			delete name;
		}
		videoMushExecute(output_count, &use_outputs);
		delete use_outputs;
	}
	over++;
	
	bool set = false;
	while (set = getLog(output, size)) {
		std::cout << output << std::endl;
	}

	/*	} catch (cl::Error & e) {
	std::cerr << "Caught Exception: " << e.what() << std::endl << _clGetErrorMessage(e.err()) << std::endl;
	return EXIT_FAILURE;
	} catch (std::exception & e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
	} catch (...) {
	std::cerr << "Unknown exception" << std::endl;
	return EXIT_FAILURE;
	}*/

	if (logThread.joinable()) {
		logThread.join();
	}

	return EXIT_SUCCESS;
}
