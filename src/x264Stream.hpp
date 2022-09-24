#ifndef X264STREAM_HPP
#define X264STREAM_HPP

#include <string>
#include <iostream>
#include <memory>
extern "C" {
	#include <inttypes.h>
	#include <x264.h>
}
#ifdef _WIN32
#include <Windows.h>
#endif
#include "ConfigStruct.hpp"

class ldrRingBuffer;
using std::shared_ptr;

class x264Stream {
public:
	x264Stream(shared_ptr<ldrRingBuffer> outBuffer, std::string fn, std::string profile, std::string preset, bool zerolatency, unsigned int width, unsigned int height, float fps, unsigned int bitrate, hdr::chromaSubsample chromaSubsample, bool * lastFrame);
	~x264Stream() {}
	void wait();
	void destroy();
	void go();
	void loadParams(hdr::x264Pass firstpass);
	
private:
	unsigned int width, height;
	unsigned int size;
	int fps;
	int bitrate;
	
	shared_ptr<ldrRingBuffer> outBuffer;
	std::string fn;

	bool * lastFrame;
	bool * over;

	int64_t * largest_pts;
    int64_t * second_largest_pts;
	
	x264_param_t * param;

	x264_t* encoder;
	FILE ** out;
	
	bool zerolatency;
	std::string preset, profile;
	char filename [256];

	hdr::chromaSubsample chromaSubsample;
	
};
#endif
