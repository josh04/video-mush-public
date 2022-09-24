//
//  aacEncoder.hpp
//  video-mush
//
//  Created by Josh McNamee on 21/01/2015.
//
//

#ifndef video_mush_aacEncoder_hpp
#define video_mush_aacEncoder_hpp

#include <mutex>
#include <condition_variable>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <Mush Core/encoderEngine.hpp>
#include "ffmpegWrapper.hpp"

class aacEncoder : public encoderEngine, public mush::ffmpegWrapper {
public:
    aacEncoder() : encoderEngine(), ffmpegWrapper(), a(), b() {
        
    }
    
    ~aacEncoder() {
        
    }
    
    virtual void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) {
        
        addItem(&a);
        addItem(&b);
        
        _codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (!_codec) {
            putLog("codec not found");
            return;
        }
        
        _trigger_frames = (unsigned int)(_sample_rate / config.fps);
        
        
        avCodecContext = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(_codec), &avcodec_close);
        
        avCodecContext->codec_id = AV_CODEC_ID_AAC;
        
        avCodecContext->bit_rate = 96000;
        
        
        avCodecContext->channels = 1;
        avCodecContext->channel_layout = av_get_default_channel_layout(1);
        avCodecContext->sample_rate = _sample_rate;
        avCodecContext->sample_fmt = AV_SAMPLE_FMT_FLTP;
        avCodecContext->frame_size = _trigger_frames/2;
        
        avCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
        
    }
    
    virtual std::shared_ptr<AVCodecContext> libavformatContext() {
        return avCodecContext;
    }
    
    void fireTrigger() {
        trigger += _trigger_frames;
        cond.notify_one();
    }
    
    void breakTrigger() {
        triggerable = false;
        cond.notify_one();
    }
    
protected:
    virtual void gather() {
        SetThreadName("aacEncoder");
    
        
        AVDictionary * _dict = NULL;
        
        avCodecContext->strict_std_compliance = -2;

        if (avcodec_open2(avCodecContext.get(), _codec, &_dict) < 0) {
			throw std::runtime_error("Avcodec_open failed in aacEncoder.");
        }
        
        
        std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame* a){ av_frame_free(&a); });
      
        
        
        avFrame->nb_samples     = 1024;
        avFrame->channel_layout = avCodecContext->channel_layout;
        avFrame->format         = avCodecContext->sample_fmt;
        avFrame->sample_rate    = avCodecContext->sample_rate;
        
        bool incoming = true;
        bool running = true;
        int i = 0, j = 0;
        
        AVPacket * packet = nullptr;
        
        av_frame_get_buffer(avFrame.get(), 0);
        
        unsigned int out_frames = 0;
        while (running) {
            
            std::unique_lock<std::mutex> l(m);
            cond.wait(l, [&]() -> bool { return trigger > 1024 || !triggerable; });
            if (trigger < 1024) {
                incoming = false;
                avFrame->nb_samples = trigger;
                trigger = 0;
            }
            
            
            if (packet == nullptr) {
                packet = (AVPacket *)inLock().get_pointer();
                memset(packet, 0, sizeof(AVPacket));
            }
            
            for (int p = 0; p < avFrame->nb_samples; ++p) {
                float * dat = (float *)(avFrame->buf[0]->data);
                dat[p] = sin((float)p/(avFrame->nb_samples * 2));
            }
            
            int got = 0;
            avFrame->pts = i*1024;
            if (triggerable || trigger > 1024) {
                trigger = trigger - 1024;
                out_frames += 1024;
                avcodec_encode_audio2(avCodecContext.get(), packet, avFrame.get(), &got);
                ++i;
            } else {
                avcodec_encode_audio2(avCodecContext.get(), packet, NULL, &got);
            }
            
            if (got) {
                inUnlock();
                packet = nullptr;
            } else {
                if (!incoming) {
                    running = false;
                }
            }
            
        }
        std::stringstream strm;
        strm << "Total audio samples: " << out_frames;
        putLog(strm.str());
        release();
    }
    
private:
    std::shared_ptr<AVCodecContext> avCodecContext = nullptr;
    std::shared_ptr<mush::ringBuffer> _outBuffer = nullptr;
    AVCodec * _codec = NULL;
    AVPacket a, b;
    
    bool triggerable = true;
    int trigger = 0;
    std::condition_variable cond;
    std::mutex m;
    
    const unsigned int _sample_rate = 44100;
    unsigned int _trigger_frames = 1470;
    //unsigned int _width = 1280, _height = 720;
    //unsigned int _bitrate = 0;
    //float _fps = 30.0f;
    
};


#endif
