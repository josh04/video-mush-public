//
//  avcodecEncoder.cpp
//  video-mush
//
//  Created by Josh McNamee on 24/06/2015.
//
//

#include <sstream>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
}

#include "yuvDepthClampProcess.hpp"
#include "yuv422to420Process.hpp"
#include <Mush Core/encoderEngine.hpp>
#include "ffmpegWrapper.hpp"
#include <Mush Core/imageProcess.hpp>

#include "avcodecEncoder.hpp"
#include "ffmpegAutoDecode.hpp"

extern void SetThreadName(const char * threadName);

avcodecEncoder::avcodecEncoder(avcodec_codec c, bool use_auto_decode) : encoderEngine(), ffmpegWrapper(), a(), b(), _c(c) {
	if (use_auto_decode) {
		_use_auto_decode = use_auto_decode;
	}
}

avcodecEncoder::~avcodecEncoder() {
    if (avCodecContext != NULL) {
        avcodec_close(avCodecContext);
        avCodecContext = NULL;
    }
}

void avcodecEncoder::init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) {
    
    _outBuffer = mush::castToImage(outBuffer);
    
    addItem((void *)&a);
    addItem((void *)&b);
    unsigned int _size;
    _outputWidth = config.width;
    _outputHeight = config.height;
    _bitrate = config.bitrate;
    _fps = config.fps;
    
    if (!config.overrideSize) {
        _outBuffer->getParams(_outputWidth, _outputHeight, _size);
    }
    
    AVCodecID codec_id;
    
    switch (_c) {
        default:
        case avcodec_codec::x264:
            codec_id = AV_CODEC_ID_H264;
            _clamp = std::make_shared<mush::yuvDepthClampProcess>(8, 1.0f, config.func);
			_422_to_420 = std::make_shared<mush::yuv422to420Process>(8);
            break;
        case avcodec_codec::x265:
            codec_id = AV_CODEC_ID_HEVC;
            _clamp = std::make_shared<mush::yuvDepthClampProcess>(10, 1.0f, config.func);
            break;
        case avcodec_codec::vpx:
            codec_id = AV_CODEC_ID_VP9;
            _clamp = std::make_shared<mush::yuvDepthClampProcess>(12, 1.0f, config.func);
            break;
        case avcodec_codec::prores:
            codec_id = AV_CODEC_ID_PRORES;
            _clamp = std::make_shared<mush::yuvDepthClampProcess>(10, 1.0f, config.func);
            break;
    }
    
    _codec = avcodec_find_encoder(codec_id);
    
    if (!_codec) {
        throw std::runtime_error("FFmpeg: Codec not found.");
    }
    
    _clamp->init(context, outBuffer);

	if (_422_to_420.get() != nullptr) {
		_422_to_420->init(context, _clamp.get());
	}
    
	avCodecContext = avcodec_alloc_context3(_codec);
    
    avCodecContext->codec_id = codec_id;
    
    avCodecContext->width = _outputWidth;
    avCodecContext->height = _outputHeight;
    
    avCodecContext->bit_rate = _bitrate;
    
    avCodecContext->time_base.den = _fps;
    avCodecContext->time_base.num = 1;
    
    switch (_c) {
        case avcodec_codec::x264:
            avCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
            avCodecContext->level = 52;
            break;
        case avcodec_codec::x265:
            avCodecContext->pix_fmt = AV_PIX_FMT_YUV420P10LE;
            avCodecContext->level = 42;
            break;
        case avcodec_codec::vpx:
            avCodecContext->pix_fmt = AV_PIX_FMT_YUV420P12LE;
            break;
        case avcodec_codec::prores:
            avCodecContext->pix_fmt = AV_PIX_FMT_YUV422P10LE;
            //avCodecContext->level = 41;
            break;
    }
	//avCodecContext->colorspace = AVCOL_SPC_BT709;
	//avCodecContext->color_range = AVCOL_RANGE_MPEG;
        
   // avCodecContext->gop_size = 10; // JOSH uhhh
    
    queue = context->getQueue();
    
    int clampImageSize = _clamp->getMapSize()*2; // clamp provides 422
    
    unsigned int size;
    _outBuffer->getParams(_inputWidth, _inputHeight, size);
    
    size_t sz = _clamp->get_buffer_int_width();
    
    ptr[0] = context->hostReadBuffer(clampImageSize);
    ptr[1] = ptr[0] + _inputWidth*_inputHeight*sz;
    ptr[2] = ptr[1] + (_inputWidth*_inputHeight/2)*sz;
    
    _crf = config.crf;
    _profile = config.h264Profile;
    _preset = config.h264Preset;
    
    _zerolatency = config.zerolatency;
    
	if (_use_auto_decode) {
        
		_auto_decode = std::make_shared<mush::ffmpegAutoDecode>(_c, config.func, _outputWidth, _outputHeight, avCodecContext);
		_auto_decode->init(context, {});

		std::string codec_name;

		switch (_c) {
		case avcodec_codec::x264:
			codec_name = "x264";
			break;
		case avcodec_codec::x265:
			codec_name = "x265";
			break;
		case avcodec_codec::vpx:
			codec_name = "VP9";
			break;
		case avcodec_codec::prores:
			codec_name = "Prores";
			break;
		}

		codec_name += " Decoded Output";

		_auto_decode->setTagInGuiName(codec_name);

	}

}

void avcodecEncoder::gather() {
    SetThreadName("avcodecEncoder");
    if (!_outBuffer) {
        putLog("avcodecEncoder received non-ldr buffer");
        return;
    }
    
    AVDictionary * _dict = NULL;
    
    size_t int_width = _clamp->get_buffer_int_width();
    
    switch (_c) {
        default:
        case avcodec_codec::x264:
            av_dict_set(&_dict, "vprofile", _profile.c_str(), 0); // Prores 422 HQ
            av_dict_set(&_dict, "preset", _preset.c_str(), 0);
            break;
        case avcodec_codec::x265:
            av_dict_set(&_dict, "vprofile", "main", 0);
            av_dict_set(&_dict, "preset", _preset.c_str(), 0);
            break;
        case avcodec_codec::vpx:
            av_dict_set(&_dict, "profile", "2", 0);
            av_dict_set(&_dict, "speed", "1", 0);
            av_dict_set(&_dict, "threads", "8", 0);
            av_dict_set(&_dict, "tile-columns", "6", 0);
            av_dict_set(&_dict, "frame-parallel", "1", 0);
            break;
        case avcodec_codec::prores:
            av_dict_set(&_dict, "vprofile", "3", 0); // Prores 422 HQ
            break;
    }
    
	if (_zerolatency) {
		if (_c == avcodec_codec::x264 || _c == avcodec_codec::x265) {
			av_dict_set(&_dict, "tune", "zerolatency", 0);
		}
    }
    
    std::stringstream crf;
    crf << _crf;
    av_dict_set(&_dict, "crf", crf.str().c_str(), 0);
    
    if (avcodec_open2(avCodecContext, _codec, &_dict) < 0) {
        return;
    }
    
    AVPixelFormat fmt_in;
    AVPixelFormat fmt_out;
    struct SwsContext * convert_context = NULL;
	size_t picture_buffer_size;

    switch (_c) {
        default:
        case avcodec_codec::x264:
            fmt_in = AV_PIX_FMT_YUV422P;
            fmt_out = AV_PIX_FMT_YUV420P;
			picture_buffer_size = (_outputWidth * _outputHeight * 3 * int_width) / 2;
            break;
        case avcodec_codec::x265:
            fmt_in = AV_PIX_FMT_YUV422P10LE;
            fmt_out = AV_PIX_FMT_YUV420P10LE;
            picture_buffer_size = (_outputWidth * _outputHeight * 3 * int_width) / 2;
            avCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
            break;
        case avcodec_codec::vpx:
            fmt_in = AV_PIX_FMT_YUV422P12LE;
            fmt_out = AV_PIX_FMT_YUV420P12LE;
			picture_buffer_size = (_outputWidth * _outputHeight * 3 * int_width) / 2;
            break;
        case avcodec_codec::prores:
            fmt_in = AV_PIX_FMT_YUV422P10LE;
            fmt_out = AV_PIX_FMT_YUV422P10LE;
			picture_buffer_size = (_outputWidth * _outputHeight * 2 * int_width);
            break;
    }

	std::vector<uint8_t> picture_buffer(picture_buffer_size);

    convert_context = sws_getContext(_inputWidth, _inputHeight, fmt_in, _outputWidth, _outputHeight, fmt_out, SWS_LANCZOS | SWS_ACCURATE_RND, NULL, NULL, NULL);
    
    std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame* a){ av_frame_free(&a); });
    
    avFrame->data[0] = picture_buffer.data();
    avFrame->data[1] = avFrame->data[0] + _outputWidth*_outputHeight*int_width;
	if (fmt_out == AV_PIX_FMT_YUV420P12LE || fmt_out == AV_PIX_FMT_YUV420P10LE || fmt_out == AV_PIX_FMT_YUV420P) {
		avFrame->data[2] = avFrame->data[1] + (_outputWidth*_outputHeight / 4)*int_width;
	} else if (fmt_out == AV_PIX_FMT_YUV422P10LE) {
		avFrame->data[2] = avFrame->data[1] + (_outputWidth*_outputHeight / 2)*int_width;
	} else {
		throw std::runtime_error("Unusual pixel format.");
	}
    avFrame->linesize[0] = _outputWidth*int_width;
    avFrame->linesize[1] = (_outputWidth / 2)*int_width;
    avFrame->linesize[2] = (_outputWidth / 2)*int_width;
    
    avFrame->format = avCodecContext->pix_fmt;
    avFrame->width = avCodecContext->width;
    avFrame->height = avCodecContext->height;
    
    bool incoming = true;
    bool running = true;
    int i = 0, j = 0;
    
    AVPacket * packet = nullptr;
    
    cl::Event event;
    
    int clampImageSize = _clamp->getMapSize()*2;
    
    while (running) {
        _clamp->process();
		if (_422_to_420.get() != nullptr) {
			_422_to_420->process();

			auto clampBuffer = _422_to_420->outLock();

			if (clampBuffer == nullptr) {
				incoming = false;
			} else {

				queue->enqueueReadBuffer(clampBuffer.get_buffer(), CL_TRUE, 0, _422_to_420->get_size(), avFrame->data[0], NULL, &event);
				//event.wait();
				/*
				int srcstride[3];
				srcstride[0] = _inputWidth*int_width;
				srcstride[1] = (_inputWidth / 2)*int_width;
				srcstride[2] = (_inputWidth / 2)*int_width;

				if (convert_context != nullptr) {
					sws_scale(convert_context, ptr, srcstride, 0, _inputHeight, avFrame->data, avFrame->linesize);
				}
				*/
				avFrame->pts = i;
				++j;
				_422_to_420->outUnlock();
			}
		} else {
			auto clampBuffer = _clamp->outLock();

			if (clampBuffer == nullptr) {
				incoming = false;
			} else {

				queue->enqueueReadBuffer(clampBuffer.get_buffer(), CL_TRUE, 0, clampImageSize, ptr[0], NULL, &event);
				//event.wait();

				int srcstride[3];
				srcstride[0] = _inputWidth*int_width;
				srcstride[1] = (_inputWidth / 2)*int_width;
				srcstride[2] = (_inputWidth / 2)*int_width;

				if (convert_context != nullptr) {
					sws_scale(convert_context, ptr, srcstride, 0, _inputHeight, avFrame->data, avFrame->linesize);
				}

				avFrame->pts = i;
				++j;
				_clamp->outUnlock();
			}
		}

        
        if (packet == nullptr) {
			auto buf = inLock();
            if (buf == nullptr) {
                break;
            }
			packet = (AVPacket *)buf.get_pointer();
            memset(packet, 0, sizeof(AVPacket));
        }
        
        int got = 0;
        avFrame->pts = i;

		AVFrame * frame_pointer = NULL;

        if (incoming) {
			frame_pointer = avFrame.get();
        }

		int out_size = avcodec_encode_video2(avCodecContext, packet, frame_pointer, &got);
		if (out_size < 0) {
			putLog("avcodec_encode_video2 error");
			break;
		}
        
        if (got) {
			{
				std::lock_guard<std::mutex> lock(_packet_size_mutex);
				_packet_sizes.push_back(packet->size);
			}
            /* Write the compressed frame to the media file. */
            inUnlock();
            packet = nullptr;
        } else {
            if (!incoming) {
                running = false;
            }
        }
        ++i;
    }
    
	avcodec_close(avCodecContext);

	avCodecContext = NULL;

	sws_freeContext(convert_context);
    
    release();
}

void avcodecEncoder::set_global_header(bool header) {
    if (header) {
        avCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
}

void avcodecEncoder::inUnlock() {
	if (_use_auto_decode && _auto_decode != nullptr) {
		AVPacket * p = (AVPacket *)_getMem(next).get_pointer();
		AVPacket q;
		memcpy(&q, p, sizeof(AVPacket));

		_auto_decode->process(&q);
	}
	encoderEngine::inUnlock();
}

std::vector<std::shared_ptr<mush::guiAccessible>> avcodecEncoder::getGuiBuffers() {
	if (_auto_decode != nullptr) {
		return{ _auto_decode };
	} else {
		return {};
	}
}

mush::metric_value avcodecEncoder::get_last() const {
	std::lock_guard<std::mutex> lock(_packet_size_mutex);
	mush::metric_value var;
    if (_packet_sizes.size() > 0) {
        var.type = mush::metric_value_type::i;
        var.value.integer = _packet_sizes.back();
    } else {
        var.type = mush::metric_value_type::null;
    }
	return var;
}
