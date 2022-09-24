//
//  ffmpegAutoDecode.cpp
//  video-mush
//
//  Created by Josh McNamee on 24/08/2016.
//
//

#include "ffmpegAutoDecode.hpp"

namespace mush {

    ffmpegAutoDecode::ffmpegAutoDecode(avcodec_codec e, transfer f, unsigned int width, unsigned int height, AVCodecContext * parent_ctx) : mush::imageProcess(), ffmpegWrapper(), _e(e), _f(f), parent_ctx(parent_ctx) {
        _width = width;
        _height = height;
    }
    
    void ffmpegAutoDecode::release() {
        
        avcodec_close(codec_ctx);
        
        imageProcess::release();
    }
    
    void ffmpegAutoDecode::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer> > buffers) {
            assert(buffers.size() == 0);
        
            queue = context->getQueue();
        
        /*
            core::outputConfigStruct config;
            config.defaults();
            config.width = _width;
            config.height = _height;
            
            config.func = _f;
            
            config.h264Preset = "slower";
            config.zerolatency = true;
            config.crf = _crf;
         */
            
            //_encoder = std::make_shared<avcodecEncoder>(_e);
            //_encoder->init(context, input, config);
            
            
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
			int_width = sizeof(uint16_t);
            
            sws_ctx = std::shared_ptr<SwsContext>(sws_getContext(_width, _height, fmt_in, _width, _height, fmt_out, SWS_LANCZOS | SWS_ACCURATE_RND, NULL, NULL, NULL), [](SwsContext * s) {sws_freeContext(s); });
            
            if (codec == nullptr) {
                throw std::runtime_error("FFmpeg: Codec not found.");
            }
            
            codec_ctx = avcodec_alloc_context3(codec);
            
            codec_ctx->extradata = parent_ctx->extradata;
            codec_ctx->extradata_size = parent_ctx->extradata_size;
            
			codec_ctx->width = _width;
			codec_ctx->height = _height;

            avcodec_open2(codec_ctx, codec, nullptr);
            
            
            //sws_setColorspaceDetails(sws_ctx.get(), sws_getCoefficients(SWS_CS_ITU709), 0, sws_getCoefficients(SWS_CS_ITU709), 1, 0, 1 << 16, 1 << 16);
            
            
            host_buffer = context->hostWriteBuffer(_width * _height * int_width * 4);
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
    
    void ffmpegAutoDecode::process(AVPacket * packet) {
        
        //cl::Event event;
        /*
        AVPacket * a = (AVPacket *)_encoder->outLock();
        if (a == nullptr) {
            release();
            return;
        }
         */
        
        AVFrame * f = av_frame_alloc();
        memset(f, 0, sizeof(AVFrame));
        
        int isFrameAvailable = false;
        int processedLength = avcodec_decode_video2(codec_ctx, f, &isFrameAvailable, packet);
        
        memset(packet, 0, sizeof(AVPacket));
        
        //_encoder->outUnlock();
        
        
        if (isFrameAvailable) {
            inLock();
            
            int dststride = _width * 4 * int_width;
            sws_scale(sws_ctx.get(), f->data, f->linesize, 0, f->height, &host_buffer, &dststride);
            
            cl::Event event;
            cl::size_t<3> origin, region;
            origin[0] = 0; origin[1] = 0; origin[2] = 0;
            region[0] = _width; region[1] = _height; region[2] = 1;
            queue->enqueueWriteImage(*upload, CL_TRUE, origin, region, 0, 0, host_buffer, NULL, &event);
            event.wait();
            
            transfer_kernel->setArg(0, *upload);
            transfer_kernel->setArg(1, _getImageMem(0));
            if (_f == transfer::pq) {
                //transfer_kernel->setArg(2, 1.0f);
            }
            queue->enqueueNDRangeKernel(*transfer_kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            inUnlock();
        }
        av_frame_free(&f);
        
    }
    
    void ffmpegAutoDecode::go() {
        
        //_encoder_thread = std::unique_ptr<std::thread>(new std::thread(&avcodecEncoder::startThread, _encoder));
        
        while (good()) {
            process(NULL);
        }
        
        //if (codec_ctx != NULL) {
        //	avcodec_free_context(&codec_ctx);
        //}
        
        release();
    }
    
}
