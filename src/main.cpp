// Correct for Canon 5D MkII
#define DEBUG
#define _CLDEBUG
#define CLTYPE CL_DEVICE_TYPE_GPU

#define __CL_ENABLE_EXCEPTIONS
/*
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include <boost/thread.hpp>
*/
#include <vector>
#include <signal.h>

#ifdef _WIN32
#include <tchar.h>
#include "../XGetopt.h"
#else
#include <getopt.h>
#endif

#include <fcntl.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/timer/timer.hpp>


#include <GL/glew.h>

#ifdef __linux__
#include <GL/glx.h>
#endif

#include "../CL/cl.hpp"

#include "DeckLinkAPI.h"

#include "../hdrContext.hpp"

#include "average.hpp"
#include "canon5d.hpp"
#include "../clerr.h"
#include "videoframe.hpp"

#include "../hdrEXR.hpp"

#include "gui.hpp"
#include "capture.hpp"

using namespace boost::assign;

const char * usage = "Usage: capture [-i|--isos 100,400,800,1600] [output.pipe]";

int main(int argc, char * argv[]) {

	//try {
//	chdir(dirname(argv[0])); // Eeeeek, not portable

	// Options
	std::vector<float> isoArray = list_of(1.0f)(4.0f)(8.0f)(32.0f);
	char c;
	unsigned int width = 1920;
	unsigned int height = 1080;

#ifndef _WIN32
	int optionIndex = 0;
	const struct option longOptions[] = {
		{"isos", required_argument, NULL, 'i'},
		{0, 0, 0, 0}
	};

	while (true) {
		if ((c = getopt_long(argc, (char * const *)argv, "i:?", longOptions, &optionIndex)) == -1) {break;}
#else
	while (true) {
		if ((c = getopt(argc, (TCHAR**)argv, "w:h:i:?")) == -1) {break;}			
#endif
		switch (c) {
			case 'i':
				{
					isoArray.clear();	
					float iso;
					std::istringstream sstream(optarg);
					for (int i = 0; i < 4; ++i) {
						sstream >> iso;
						sstream >> c;
						isoArray.push_back(iso);
					}
				}
				break;
			case 'w': width = atoi(optarg); break;
			case 'h': height = atoi(optarg); break;
			case '?': std::cout << usage << std::endl; return EXIT_SUCCESS; break;
		}
	}

	argc = argc - optind + 1; // This is potentially a terrible terrible idea
	argv = argv + optind - 1;

	// OpenCL+GL Setup

	size_t hdrFrameSize = sizeof(char) * 3 * width * height;
	void * ptrImage =  malloc(hdrFrameSize);
	memset(ptrImage, 0, hdrFrameSize);

	hdrGui * gui = new hdrGui(ptrImage, width, height);

	gui->init();
	
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

//	GLXContext glCtx = glXGetCurrentContext();
	HGLRC glCtx =  wglGetCurrentContext();
	HDC glHDC2 = wglGetCurrentDC();
	
	cl_context_properties props[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(),
		CL_GL_CONTEXT_KHR, (cl_context_properties)glCtx,
		CL_WGL_HDC_KHR, (cl_context_properties)glHDC2,
		0
	};
	
	hdrContext * context = new hdrContext(CLTYPE, props);
	context->addKernels(hdrCapture::listKernels());
	context->addKernels(hdrEXR::clToEXR::listKernels());
	context->addKernels(Average::listKernels());
	context->makeProgram();

	hdrCapture * capture = new hdrCapture(context, gui, isoArray, argv[1]);
	capture->start();
	bool done = false;
	int i;
	while(!done) {
		capture->capture();
		i = gui->events();
		switch(i) {
			case _EV_OKAY: break;
			case _EV_EXIT: done = true; break;
			case _EV_EXR: capture->writeEXR(); break;
			case _EV_EXRS: capture->writeEXRs(); break;
			case _EV_RESET: capture->resetOffsets(); break;

			case _EV_LEFT: capture->pushOffset(gui->offsetIndex, _EV_LEFT); break;
			case _EV_RIGHT: capture->pushOffset(gui->offsetIndex, _EV_RIGHT); break;
			case _EV_UP: capture->pushOffset(gui->offsetIndex, _EV_UP); break;
			case _EV_DOWN: capture->pushOffset(gui->offsetIndex, _EV_DOWN); break;
		}
	}
	capture->stop();

	delete capture;
	delete gui;
	delete context;


	return EXIT_SUCCESS;
}

