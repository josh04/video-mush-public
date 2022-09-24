//
//  ffmpegEncodeDecode.hpp
//  parcore
//
//  Created by Josh McNamee on 24/06/2015.
//  Copyright (c) 2015 Josh McNamee. All rights reserved.
//

#ifndef parcore_ffmpegAutoDecode_hpp
#define parcore_ffmpegAutoDecode_hpp

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}


#ifndef __APPLE__
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>
#include "avcodecEncoder.hpp"
#include "ConfigStruct.hpp"
#else

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>
#include <Video Mush/avcodecEncoder.hpp>
#include <Video Mush/ConfigStruct.hpp>
#endif


namespace mush {
	class ffmpegAutoDecode : public mush::imageProcess, public ffmpegWrapper {
	public:

        ffmpegAutoDecode(avcodec_codec e, transfer f, unsigned int width, unsigned int height, AVCodecContext * parent_ctx);

		~ffmpegAutoDecode() {
		}

        void release();

        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
        
		void process() override {
			process(NULL);
		}

        void process(AVPacket * packet);

        void go();

	private:
		cl::CommandQueue * queue = nullptr;

		//std::shared_ptr<encoderEngine> _encoder = nullptr;

		//std::unique_ptr<std::thread> _encoder_thread = nullptr;

		AVCodecContext * codec_ctx = NULL, * parent_ctx = NULL;

		std::shared_ptr<SwsContext> sws_ctx = nullptr;

		cl::Image2D * upload = nullptr;
		cl::Kernel * transfer_kernel = nullptr;
		uint8_t * host_buffer = nullptr;

		avcodec_codec _e;
		transfer _f;

		size_t int_width;
	};
}

#endif
