//
//  ffmpegEncodeDecode.hpp
//  parcore
//
//  Created by Josh McNamee on 24/06/2015.
//  Copyright (c) 2015 Josh McNamee. All rights reserved.
//

#ifndef parcore_ffmpegEncodeDecode_hpp
#define parcore_ffmpegEncodeDecode_hpp

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
}


#ifndef __APPLE__
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>
#include <Mush Core/registerContainer.hpp>
#include "avcodecEncoder.hpp"
#include "ConfigStruct.hpp"
#else

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Video Mush/avcodecEncoder.hpp>
#include <Video Mush/ConfigStruct.hpp>
#endif


namespace mush {
    class ffmpegEncodeDecode : public mush::imageProcess, public ffmpegWrapper {
    public:
        
        ffmpegEncodeDecode(avcodec_codec e, transfer f, int crf = 24) : mush::imageProcess(), ffmpegWrapper(), _e(e), _crf(crf), _f(f) {
        }
        
        ~ffmpegEncodeDecode() {
		}
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            auto input = castToImage(buffers.begin()[0]);
            
            input->getParams(_width, _height, _size);
            
            queue = context->getQueue();
            
            core::outputConfigStruct config;
            config.defaults();
            config.width = _width;
            config.height = _height;
            
            config.func = _f;
            
            config.h264Preset = "slower";
            config.zerolatency = true;
            config.crf = _crf;
            
            config.use_auto_decode = false;
            
            _encoder = std::make_shared<avcodecEncoder>(_e, false);
            _encoder->init(context, input, config);
            
            
            addItem(context->floatImage(_width, _height));
            
            AVCodec * codec = nullptr;
            
            AVPixelFormat fmt_in, fmt_out;
            
            switch (_e) {
                case avcodec_codec::x264:
                    codec = avcodec_find_decoder_by_name("h264");
                    fmt_in = AV_PIX_FMT_YUV420P;
                    break;
                case avcodec_codec::x265:
                    codec = avcodec_find_decoder_by_name("hevc");
                    fmt_in = AV_PIX_FMT_YUV420P10LE;
                    break;
                case avcodec_codec::vpx:
                    codec = avcodec_find_decoder_by_name("vp9");
                    fmt_in = AV_PIX_FMT_YUV420P12LE;
                    break;
                case avcodec_codec::prores:
                    codec = avcodec_find_decoder_by_name("prores");
                    fmt_in = AV_PIX_FMT_YUV422P10LE;
                    break;
            }
            
            fmt_out = AV_PIX_FMT_RGBA64;
            
            sws_ctx = std::shared_ptr<SwsContext>(sws_getContext(_width, _height, fmt_in, _width, _height, fmt_out, SWS_LANCZOS | SWS_ACCURATE_RND, NULL, NULL, NULL), [](SwsContext * s) {sws_freeContext(s); });
            
            if (codec == nullptr) {
                throw std::runtime_error("FFmpeg: Codec not found.");
            }
            
            codec_ctx = avcodec_alloc_context3(codec);
            
            auto e_ctx = _encoder->libavformatContext();
            codec_ctx->extradata = e_ctx->extradata;
            codec_ctx->extradata_size = e_ctx->extradata_size;
            
            codec_ctx->width = _width;
            codec_ctx->height = _height;
            
            avcodec_open2(codec_ctx, codec, nullptr);
            
            
            //sws_setColorspaceDetails(sws_ctx.get(), sws_getCoefficients(SWS_CS_ITU709), 0, sws_getCoefficients(SWS_CS_ITU709), 1, 0, 1 << 16, 1 << 16);
            
            
            host_buffer = context->hostWriteBuffer(_width*_height*sizeof(uint16_t)*4);
            upload = context->int16bitImage(_width, _height);
            
            switch (_f) {
                case transfer::g8:
                    transfer_kernel = context->getKernel("decodePTF4");
                    transfer_kernel->setArg(2, 1.0f);
                    break;
                case transfer::pq:
                    transfer_kernel = context->getKernel("pqdecode");
                    //transfer_kernel->setArg(2, 1.0);
                    break;
                case transfer::srgb:
                    transfer_kernel = context->getKernel("decodeSRGB");
                    break;
                case transfer::gamma:
                    transfer_kernel = context->getKernel("gamma");
                    break;
                case transfer::rec709:
                    transfer_kernel = context->getKernel("decodeRec709");
                    break;
                default:
                case transfer::linear:
                    transfer_kernel = nullptr;
                    break;
            }
            
        }
        
        void process() {
            
            //cl::Event event;
			auto bud = _encoder->outLock();
			if (bud == nullptr) {
				release();
				return;
			}
            AVPacket * a = (AVPacket *)bud.get_pointer();
            
            AVFrame * f = av_frame_alloc();
            memset(f, 0, sizeof(AVFrame));
            
            int isFrameAvailable = false;
            int processedLength = avcodec_decode_video2(codec_ctx, f, &isFrameAvailable, a);
            
            memset(a, 0, sizeof(AVPacket));
            _encoder->outUnlock();
            
            
            if (isFrameAvailable) {
                inLock();
                
                int dststride = _width*4*sizeof(uint16_t);
                sws_scale(sws_ctx.get(), f->data, f->linesize, 0, f->height, &host_buffer, &dststride);
                
                cl::Event event;
                cl::size_t<3> origin, region;
                origin[0] = 0; origin[1] = 0; origin[2] = 0;
                region[0] = _width; region[1] = _height; region[2] = 1;
                queue->enqueueWriteImage(*upload, CL_TRUE, origin, region, 0, 0, host_buffer, NULL, &event);
                event.wait();
                
                transfer_kernel->setArg(0, *upload);
                transfer_kernel->setArg(1, _getImageMem(0));
                /*if (_f == transfer::pq) {
                    transfer_kernel->setArg(2, 1.0f);
                }*/
                queue->enqueueNDRangeKernel(*transfer_kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
                event.wait();
                inUnlock();
            }
			av_frame_free(&f);
            
        }
        
        void go() {
            
            std::unique_ptr<std::thread> _encoder_thread = std::unique_ptr<std::thread>(new std::thread(&avcodecEncoder::startThread, _encoder));
            
            while(good()) {
                process();
            }

			//if (codec_ctx != NULL) {
			//	avcodec_free_context(&codec_ctx);
			//}

			avcodec_close(codec_ctx);
            release();
            
            //_encoder->kill();
            
            try {
                if (_encoder_thread != nullptr) {
                    if (_encoder_thread->joinable()) {
                        _encoder_thread->join();
                    }
                }
                _encoder_thread.reset();
            } catch (std::system_error& e) {
                putLog(e.what());
            } catch (std::exception& e) {
                putLog(e.what());
            }
        }

		std::shared_ptr<avcodecEncoder> get_encoder() const {
			return _encoder.get();
		}

    private:
        cl::CommandQueue * queue = nullptr;
        
        mush::registerContainer<avcodecEncoder> _encoder;
        
        //std::unique_ptr<std::thread> _encoder_thread = nullptr;
        
        AVCodecContext * codec_ctx = NULL;
        
        std::shared_ptr<SwsContext> sws_ctx = nullptr;
        
        cl::Image2D * upload = nullptr;
        cl::Kernel * transfer_kernel = nullptr;
        uint8_t * host_buffer = nullptr;
        
        int _crf = 24;
        
        avcodec_codec _e;
        transfer _f;
        
    };
}

#endif
