//
//  metadataEncoder.hpp
//  video-mush
//
//  Created by Josh McNamee on 08/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_metadataEncoder_hpp
#define media_encoder_metadataEncoder_hpp

#include <Mush Core/encoderEngine.hpp>
#include "averagesProcess.hpp"

extern "C" {
#include "libavcodec/avcodec.h"
}
    
class metadataEncoder : public encoderEngine {
public:
    metadataEncoder() : encoderEngine() {
        
    }
    
    ~metadataEncoder() {
        
    }
    
    virtual void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) {
        
        _metadata = std::dynamic_pointer_cast<averagesProcess>(outBuffer);
        
        if (!_metadata) {
            putLog("metadataEncoder received non-metadata buffer");
            return;
        }
        
        memset(&a, 0, sizeof(AVPacket));
        memset(&b, 0, sizeof(AVPacket));
        
        addItem(&a);
        addItem(&b);
        
        
        _fps = config.fps;
        
        dataContext = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(NULL), &avcodec_close);
        dataContext->codec_type = AVMEDIA_TYPE_DATA;
        dataContext->codec_id = AV_CODEC_ID_MPEG4SYSTEMS;
        dataContext->time_base.den = _fps;
        dataContext->time_base.num = 1;
        dataContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
        //time_base = dataContext->time_base;
    }
    
    virtual std::shared_ptr<AVCodecContext> libavformatContext() {
        return dataContext;
    }
    
    const AVCodec * codec() {
        return NULL;
    }
    
protected:
    virtual void gather() {
        SetThreadName("metadataEncoder");
        
        bool incoming = true;
        bool running = true;
        int i = 0;
        
        AVPacket * packet = nullptr;
        
        while (running) {
			auto buf = _metadata->outLock();
            if (buf == nullptr) {
                running = false;
                break;
            }
			std::array<float, 4> * adta = (std::array<float, 4> *)buf.get_pointer();
//            putLog("dumped a meta");
            
            
            /* Write the compressed frame to the media file. */
			auto buf2 = inLock();
            if (buf2 == nullptr) {
                running = false;
                break;
            }
			AVPacket * _dataPacket = (AVPacket *)buf2.get_pointer();
            
            free(_dataPacket->data);
            
            memset(_dataPacket, 0, sizeof(AVPacket));
            
            _dataPacket->pts = i;
            _dataPacket->dts = i;
            _dataPacket->data = (uint8_t*)malloc(sizeof(float)*4);
            float * tmp = (float *)_dataPacket->data;
            tmp[0] = (*adta)[0];
            tmp[1] = (*adta)[1];
            tmp[2] = (*adta)[2];
            tmp[3] = (*adta)[3];
            
            inUnlock();
            _metadata->outUnlock();
            
            ++i;
            
#ifdef _VIDEO_MUSH_FFMPEG_DEBUG
            std::stringstream strm;
            strm << boost::str(boost::format("%12i ") % i) << "Frame Encoded";
            putLog(strm.str().c_str());
#endif
        }
        
        release();

    }
    
private:
    std::shared_ptr<AVCodecContext> dataContext;
    std::shared_ptr<mush::ringBuffer> _metadata = nullptr;
    AVPacket a, b;
    float _fps = 30.0f;
};

#endif
