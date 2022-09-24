#ifndef HDRCONTROL_HPP
#define HDRCONTROL_HPP

#include <boost/thread.hpp>
#include <memory>
#include <atomic>
#include <thread>

#include "ConfigStruct.hpp"

using std::shared_ptr;

namespace cl {
    class Context;
}

namespace azure {
    class Eventable;
}

namespace mush {
    class opencl;
    class ringBuffer;
    class imageProcessor;
    class frameGrabber;
    class frameStepper;
    class guiAccessible;
    
    class quitEventHandler;
    class stepperEventHandler;
    class imageProcess;
	namespace gui {
		class fake_window;
	}
}

class hdrFlare10;


class outputEngine;

class exportInput;
class encoderEngine;
class tagInGui;




/*
 
 hdrControl.hpp
 
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
class hdrControl {
public:
	hdrControl(mush::config config);
	~hdrControl() {}

    void internalCreateContext();
    void precreateGLContext();
    void externalCreateContext(cl::Context * ctx);
    
    std::shared_ptr<mush::ringBuffer> addInput();
	std::shared_ptr<mush::imageProcess> merge(std::initializer_list<std::shared_ptr<mush::ringBuffer>> inBuffs);

	std::vector<std::shared_ptr<mush::ringBuffer>> process_init(std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffs, std::shared_ptr<mush::imageProcessor> hg = nullptr);
	void process_launch_thread();
	void encode(std::vector<std::shared_ptr<mush::ringBuffer>> outBuffers);
    void output(std::vector<std::shared_ptr<mush::ringBuffer>> outBuffers);
	void gui();

	void destroy();
    
    static const std::vector<std::string> listKernels() {
        std::vector<std::string> kernels;
        kernels.push_back("rgba");
        kernels.push_back("luminance");
        kernels.push_back("bilateral");
        kernels.push_back("averages");
        kernels.push_back("tonemap");
        kernels.push_back("merge");
        kernels.push_back("planarRGBtoRGBA");
        kernels.push_back("decodeUDP");
        kernels.push_back("clamp");
        kernels.push_back("slic");
        kernels.push_back("exposure");
        kernels.push_back("waveform");
        kernels.push_back("laplace");
        kernels.push_back("tonemap-output");
        kernels.push_back("capture");
        kernels.push_back("exr");
		kernels.push_back("sand");
		kernels.push_back("trayrace");
        kernels.push_back("intBufferToImage");
        kernels.push_back("algebra");
        kernels.push_back("falseColour");
        kernels.push_back("pq");
        kernels.push_back("barcode");
        kernels.push_back("logc");
        kernels.push_back("PTF4");
        kernels.push_back("chromaSwap");
        return kernels;
    }
    
	mush::config config;
    
    std::shared_ptr<mush::ringBuffer> getLastInput();

	std::shared_ptr<mush::opencl> getContext() {
		return context;
	}
    
    void rekernel();
    
    void addEventHandler(std::shared_ptr<azure::Eventable> ev);
    void removeEventHandler(std::shared_ptr<azure::Eventable> ev);
    
private:    
    std::shared_ptr<mush::ringBuffer> preprocess(std::shared_ptr<mush::frameGrabber> inputBuffer);

//	void encodeScreen();

	void flareEvents(int i);


	std::shared_ptr<mush::gui::fake_window> _fake_window = nullptr;
	shared_ptr<mush::opencl> context = nullptr;

    std::vector<shared_ptr<hdrFlare10>> flare10s;

    std::vector<shared_ptr<mush::frameGrabber>> inputBuffers;
	std::vector<shared_ptr<mush::ringBuffer>> preprocessBuffers;
    
    shared_ptr<mush::imageProcessor> mergeBuffer = nullptr;
    shared_ptr<mush::imageProcessor> mlvBuffer = nullptr;
    shared_ptr<mush::imageProcessor> demoMode = nullptr;
	shared_ptr<mush::imageProcessor> encoder = nullptr;

//	shared_ptr<screenStream> screen = nullptr;
    
    std::vector<shared_ptr<encoderEngine>> encodeEngines;
    shared_ptr<outputEngine> out = nullptr;
    
    shared_ptr<tagInGui> tagGui = nullptr;
    
    std::vector<std::shared_ptr<mush::guiAccessible>> guiBuffers;

    std::vector<std::thread *> inThreads, preprocessThreads;
	std::thread *decodeThread = nullptr, *mergeThread = nullptr, *processThread = nullptr;
    std::vector<std::thread *> encodeThreads;
    boost::thread *outThread = nullptr;
    
    bool quit = false;
    
	std::atomic_bool destroyed;
    
    std::vector<shared_ptr<mush::frameStepper>> steppers;
    std::shared_ptr<mush::quitEventHandler> quitter = nullptr;
    std::shared_ptr<mush::stepperEventHandler> stepperHandler = nullptr;
    
    bool guiInitialised = false;
    std::vector<std::shared_ptr<azure::Eventable>> handlers;
};


#endif
