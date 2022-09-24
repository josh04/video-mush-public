#ifndef HDRPREPROCESS_HPP
#define HDRPREPROCESS_HPP


extern void SetThreadName(const char* threadName);

#include <stdint.h>
#include <memory>
#include "ConfigStruct.hpp"

#include "dll.hpp"

//#include <Mush Core/opencl.hpp>
#include <azure/eventable.hpp>
#include <azure/chrono.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class CubeLUT;

namespace cl {
    class Image2D;
    class Image3D;
    class Buffer;
    class Kernel;
}

namespace mush {
    class opencl;
    class frameGrabber;
    class scrubbableFrameGrabber;
    
    VIDEOMUSH_EXPORTS class mushPreprocessor : public mush::imageProcess, public azure::Eventable {
public:
        mushPreprocessor(const core::inputConfigStruct& c) : mush::imageProcess(), azure::Eventable(), config(c) {
        //setTagInGuiName("Preprocessed Input");

			_previous_clock = azure::Clock::now();
	}

	~mushPreprocessor() {
        
	}
	using mush::initNode::init;
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
	void go();
    void process() override;
    
    void inUnlock() override;
    
    void setTransferFuncParameters();
    
    bool event(std::shared_ptr<azure::Event> event) override;
private:
	core::inputConfigStruct config;
    mush::transfer _func;
    const char * resourceDir;
        mush::registerContainer<mush::frameGrabber> _input;
    shared_ptr<mush::scrubbableFrameGrabber> _scrub = nullptr;

	cl::Buffer * rgb16Buffer = nullptr;
	cl::Buffer * udpBuffer = nullptr;
	cl::Image2D * rgbaInput = nullptr;
	cl::Image2D * rgba8bit = nullptr;

    cl::Image2D * output_frame = nullptr;
    cl::Image2D * resize_frame = nullptr;

	cl::Kernel * rgba = nullptr;
	cl::Kernel * planarRGBtoRGBA = nullptr;
	cl::Kernel * planarYUVtoRGBA = nullptr;
	cl::Kernel * convert = nullptr;
	cl::Kernel * bgraToRgba = nullptr;
	cl::Kernel * decodeUDP = nullptr;
	cl::Kernel * convert10 = nullptr;
    cl::Kernel * decodeMLV = nullptr;
    //cl::Kernel * debayer = nullptr;
    
    cl::Kernel * resize_kernel = nullptr;
    cl::Kernel * transfer_kernel = nullptr;
	cl::Kernel * exposure_kernel = nullptr;
    
    cl::Image3D * _cube_image = nullptr;


	mush::inputEngine _inputEngine;
	mush::filetype _filetype;
    
    std::shared_ptr<CubeLUT> _cube = nullptr;
	
	unsigned int _length = 0;
    
    void const * _lastFrame = nullptr;
    
    unsigned int _input_width, _input_height;
    
    float gammacorrect = 1.0f;
	float _darken = 0.0f;
        
    bool lock_fps = false;
    float frame_time = 0.04;

	azure::TimePoint _previous_clock;
};

}
#endif
