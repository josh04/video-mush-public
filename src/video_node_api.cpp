//
//  video_node_api.cpp
//  video-mush
//
//  Created by Josh McNamee on 23/07/2017.
//
//

#include <boost/asio.hpp>

#include "flare10capture.hpp"

#include "video_node_api.hpp"

#include <Mush Core/node_api.hpp>
#include <Mush Core/nodeManager.hpp>
#include <Mush Core/frameGrabber.hpp>
#include <Mush Core/tagInGui.hpp>
#include <Mush Core/mushWindowInterface.hpp>

#include "hdrFileBuffer.hpp"
#include "videoFileInput.hpp"
#include "rgb16bitInput.hpp"
#include "hdrEXRInputBuffer.hpp"
#include "udpFrameBuffer.hpp"
#include "singleEXRInput.hpp"
#include "hdrJpegInput.hpp"
#include "canonInput.hpp"
#include "exportInput.hpp"
#include "mlvRawInput.hpp"
#include "yuv10bitInput.hpp"
#include "mushPreprocessor.hpp"

#ifdef MUSH_DX
#include <Mush Core/dxContext.hpp>
#endif

#include <unordered_map>
/*
namespace std
{
    template<>
    struct hash<mush::inputEngine> {
        size_t operator()(const mush::inputEngine &pt) const {
            return std::hash<int>()((int)pt);
        }
    };
}
*/
uint32_t vnode_add_new_preprocessor(std::shared_ptr<mush::frameGrabber> inputBuffer, const mush::core::inputConfigStruct& input_config);

bool _types_registered = false;
std::unordered_map<mush::inputEngine, uint32_t> _input_types;
uint32_t _preprocessor_type = -1;

void vnode_if_register_types() {
	if (!_types_registered) {
		_types_registered = true;

		_input_types[mush::inputEngine::folderInput] = node_register_external_node_type("Folder");
		_input_types[mush::inputEngine::videoInput] = node_register_external_node_type("Video");
		_input_types[mush::inputEngine::rgb16bitInput] = node_register_external_node_type("RGB16");
		_input_types[mush::inputEngine::flareInput] = node_register_external_node_type("Flare");
		_input_types[mush::inputEngine::flare10Input] = node_register_external_node_type("Flare");
		_input_types[mush::inputEngine::fastEXRInput] = node_register_external_node_type("Fast EXR");
		_input_types[mush::inputEngine::udpInput] = node_register_external_node_type("UDP");
		_input_types[mush::inputEngine::testCardInput] = node_register_external_node_type("Testcard");
		_input_types[mush::inputEngine::jpegInput] = node_register_external_node_type("JPEG");
		_input_types[mush::inputEngine::externalInput] = node_register_external_node_type("Ext.");
		_input_types[mush::inputEngine::canonInput] = node_register_external_node_type("Canon");
		_input_types[mush::inputEngine::yuv10bitInput] = node_register_external_node_type("YUV10");
		_input_types[mush::inputEngine::singleEXRInput] = node_register_external_node_type("Single EXR");
		_input_types[mush::inputEngine::mlvRawInput] = node_register_external_node_type("MLV RAW");
		_input_types[mush::inputEngine::noInput] = node_register_external_node_type("Null Input");

		_preprocessor_type = node_register_external_node_type("Preprocessor");
	}
}

vnode_add_new_input_return vnode_add_new_input(mush::core::inputConfigStruct input_config) {
	vnode_if_register_types();
    std::shared_ptr<mush::frameGrabber> inputBuffer = nullptr;
    
    boost::filesystem::path inP = input_config.inputPath;
    
    if (inP.extension() == ".mlv" || inP.extension() == ".MLV") {
        input_config.inputEngine = mush::inputEngine::mlvRawInput;
    }
    
	std::string test_card = std::string(input_config.resourceDir) + std::string(input_config.testCardPath);
    
    switch(input_config.inputEngine) {
        case mush::inputEngine::folderInput:
            inputBuffer = make_shared<hdrFileBuffer>();
            std::dynamic_pointer_cast<hdrFileBuffer>(inputBuffer)->moveToFrame(input_config.frame_skip);
            break;
        case mush::inputEngine::videoInput:
            inputBuffer = make_shared<VideoFileInput>(true);
            break;
        case mush::inputEngine::rgb16bitInput:
            inputBuffer = make_shared<RGB16bitInput>();
            break;
        case mush::inputEngine::flareInput:
        case mush::inputEngine::flare10Input:
            //flare10s.push_back();
            //inputBuffer = std::dynamic_pointer_cast<mush::frameGrabber>(flare10s.back());
            inputBuffer = std::dynamic_pointer_cast<mush::frameGrabber>(make_shared<hdrFlare10>());
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
            input_config.inputPath = test_card.c_str();
            inputBuffer = make_shared<mush::singleEXRInput>(test_card);
            break;
        case mush::inputEngine::jpegInput:
            inputBuffer = make_shared<hdrJpegInput>();
            break;
        case mush::inputEngine::externalInput:
            inputBuffer = make_shared<exportInput>();
            break;
        case mush::inputEngine::canonInput:
            input_config.gammacorrect = 2.2f;
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
            auto buf = make_shared<mush::mlvRawInput>(input_config.blackPoint, 0, input_config.frame_skip);
            inputBuffer = buf;
            //addEventHandler(buf);
        }
            break;
        default:
        case mush::inputEngine::noInput:
            inputBuffer = nullptr;
			return { (uint32_t)-1 };
	};


	uint32_t input_type = -1;
	auto search = _input_types.find(input_config.inputEngine);
	if (search != _input_types.end()) {
		input_type = search->second;
	}

	uint32_t i_id = node_register_external_node(inputBuffer, input_type);
    //inputBuffer->init(context, input_config);
    //inputBuffer->getDetails(input_config);
    
    //inThreads.push_back(new std::thread(&mush::frameGrabber::startThread, inputBuffer));
    
    uint32_t pre_id = vnode_add_new_preprocessor(inputBuffer, input_config);
	uint32_t processor_id = node_create_processor();
	node_add_node_to_processor(processor_id, i_id);
	node_add_node_to_processor(processor_id, pre_id);

	node_add_node_to_link(pre_id, i_id);

	return{ processor_id, i_id, pre_id };
}

std::vector<std::thread> _input_threads;

VIDEOMUSH_EXPORTS bool vnode_spawn_input_thread(uint32_t i_id) {
	auto manager = node_get_node_manager();

	auto input = manager->get_cast_node<mush::frameGrabber>(i_id);
	if (input != nullptr) {
		_input_threads.push_back(std::thread(&mush::frameGrabber::startThread, input));
		return true;
	}
	return false;
}


uint32_t vnode_add_new_preprocessor(std::shared_ptr<mush::frameGrabber> inputBuffer, const mush::core::inputConfigStruct& input_config) {
	std::shared_ptr<mush::mushPreprocessor> preprocessBuffer = make_shared<mush::mushPreprocessor>(input_config);
	//preprocessBuffer->init(context, inputBuffer, input_config);

	uint32_t pre_id = node_register_external_node(preprocessBuffer, _preprocessor_type);
	return pre_id;

	//preprocessThreads.push_back(new std::thread(&mush::mushPreprocessor::preprocess, preprocessBuffer));
	/*
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
	} else {*/
		//preprocessBuffer->rocketGUI("scrub.rml");
		//addEventHandler(preprocessBuffer);
	//}
}

#ifdef MUSH_DX
std::shared_ptr<mush::dx> _dx = nullptr;
#endif
std::shared_ptr<tagInGui> _tg = nullptr;


#ifdef MUSH_DX
void vnode_create_dx_context(HWND hwnd) {
	auto scarlet_interface = std::make_shared<mush::gui::window_interface>("");

	uint32_t flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	auto window = scarlet_interface->createWindow("", 1, 1, false, flags);
	auto context = scarlet_interface->createContext(window);

	//_tg = make_shared<tagInGui>();
	//_tg->createGLContext(false, false, "");


	_dx = std::make_shared<mush::dx>();
	_dx->init(hwnd);
}

bool vnode_has_dx_context() {
	return (_dx != nullptr);
}

void * vnode_get_dx_texture() {
	return _dx->make_texture();
}
#endif
