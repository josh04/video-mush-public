//
//  x264Encoder.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_x264Encoder_hpp
#define media_encoder_x264Encoder_hpp

struct AVCodecContext;
struct AVCodec;
struct AVPacket;
namespace mush {
    class imageProcess;
    class integerMapProcess;
    class yuvDepthClampProcess;
	class ffmpegAutoDecode;
	class yuv422to420Process;
}

#include <mutex>

#include "dll.hpp"

#include <Mush Core/encoderEngine.hpp>
#include "ffmpegWrapper.hpp"
#include <Mush Core/metricReporter.hpp>
#include <Mush Core/registerContainer.hpp>


enum class avcodec_codec {
    x264,
    x265,
    vpx,
    prores
};

class avcodecEncoder : public encoderEngine, public mush::ffmpegWrapper, public mush::metricReporter {
public:
	VIDEOMUSH_EXPORTS avcodecEncoder(avcodec_codec c, bool use_auto_decode);
    
    VIDEOMUSH_EXPORTS ~avcodecEncoder();
    
	VIDEOMUSH_EXPORTS virtual void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) override;
    
	VIDEOMUSH_EXPORTS virtual std::shared_ptr<AVCodecContext> libavformatContext() override {
		return std::shared_ptr<AVCodecContext>(avCodecContext, [](AVCodecContext * c) {});
    }
    
    void set_global_header(bool header);
    
	void inUnlock() override;

	std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;

	std::string get_title_format_string() const override {
		return " %9s | ";
	}

	std::string get_format_string() const override {
		return " %8d b | ";
	}

	mush::metric_value get_last() const override;

protected:
    virtual void gather() override;

private:
    AVCodecContext * avCodecContext = NULL;
    std::shared_ptr<mush::imageBuffer> _outBuffer = nullptr;
    AVCodec * _codec = NULL;
    AVPacket a, b;
    
    unsigned int _bitrate = 0;
    float _fps = 30.0f;
    cl::CommandQueue * queue = nullptr;
    
    uint8_t * ptr[3];
    
    mush::registerContainer<mush::yuvDepthClampProcess> _clamp;
	mush::registerContainer<mush::yuv422to420Process> _422_to_420;
    
    avcodec_codec _c;
    
    int _crf = 24;
    std::string _profile = "high";
    std::string _preset = "faster";
    bool _zerolatency = "";

	bool _use_auto_decode = false;
	std::shared_ptr<mush::ffmpegAutoDecode> _auto_decode = nullptr;

	mutable std::mutex _packet_size_mutex;
	int last_packet_size;
	std::vector<size_t> _packet_sizes;
    
    size_t _token = -1;
};

#endif
