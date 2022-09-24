//
//  libavformatOutput.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_libavformatOutput_hpp
#define media_encoder_libavformatOutput_hpp

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

#include "aacEncoder.hpp"
#include <Mush Core/encoderEngine.hpp>
#include <Mush Core/outputEngine.hpp>
#include <Mush Core/checkExists.hpp>

class libavformatOutput : public outputEngine {
public:
    libavformatOutput() : outputEngine() {
        
    }
    
    ~libavformatOutput() {
        
    }
    
    virtual void init(std::vector<std::shared_ptr<encoderEngine>> encoderEngines, mush::core::outputConfigStruct config) {
        _encoders = encoderEngines;
        
        //auto _aac = make_shared<aacEncoder>();
        //_aac->init(nullptr, nullptr, config);
        //_encoders.push_back(_aac);
        
        //auto ptr = new std::thread(&encoderEngine::startThread, _aac);
        
        _width = config.width;
        _height = config.height;
        //fps = outputConfig.fps;
        
        filename = config.outputName;
        path = config.outputPath;
        
        _liveStream = config.liveStream;
        
        for (auto& enc : encoderEngines) {
            enc->addRepeat();
        }
        
        switch (config.encodeEngine) {
            case mush::encodeEngine::x265:
            case mush::encodeEngine::x264:
            default:
                container_string = "mp4";
                break;
            case mush::encodeEngine::vpx:
                container_string = "webm";
                break;
            case mush::encodeEngine::prores:
                container_string = "mov";
                break;
        }
    }
    
    virtual void gather() {
        
        AVFormatContext * _format = nullptr;
        
        std::stringstream local_web_root;
        
        if (_liveStream.enabled) {
            local_web_root << _liveStream.webroot << "/" << _liveStream.streamname;
			avformat_alloc_output_context2(&_format, NULL, NULL, local_web_root.str().c_str());
        } else {
            avformat_alloc_output_context2(&_format, NULL, container_string, NULL);
        }
        
        avFormat = std::shared_ptr<AVFormatContext>(_format, &avformat_free_context);
        AVOutputFormat * outputFormat = avFormat->oformat;
        
        std::vector<AVStream *> videoStreams;
        std::vector<bool> runnings;
        for (auto encoder : _encoders) {
            auto ctxPtr = encoder->libavformatContext();
            if (ctxPtr != nullptr) {
                videoStreams.push_back(avformat_new_stream(avFormat.get(), ctxPtr->codec));
                videoStreams.back()->codec = ctxPtr.get();
            } else {
                videoStreams.push_back(avformat_new_stream(avFormat.get(), NULL));
            }
            videoStreams.back()->id = avFormat->nb_streams - 1;
            
            
            auto cptr = std::dynamic_pointer_cast<avcodecEncoder>(encoder);
            if (cptr != nullptr) {
                cptr->set_global_header(avFormat->oformat->flags & AVFMT_GLOBALHEADER);
            }
            
            runnings.push_back(true);
        }
        
        
        
        //runnings.back() = false; // infinite aac
        
        //avFormat->sample_rate = 44100;
        //avFormat->time_base = _encoders[0]->time_base;
        
        if (_liveStream.enabled) {
            av_opt_set_double(avFormat->priv_data, "hls_time", _liveStream.file_length, AV_OPT_SEARCH_CHILDREN);
            av_opt_set_int(avFormat->priv_data, "hls_list_size", _liveStream.num_files, AV_OPT_SEARCH_CHILDREN);
            av_opt_set_int(avFormat->priv_data, "hls_wrap", _liveStream.wrap_after, AV_OPT_SEARCH_CHILDREN);
            av_opt_set(avFormat->priv_data, "hls_base_url", _liveStream.webaddress, AV_OPT_SEARCH_CHILDREN);
        
        }
        //av_dump_format(m_formatContext, 0, filename, 1);
        
        std::stringstream final;
        final << path << "/" << filename;
        std::string fullPath = mush::checkExists(final.str());
        
        if (!(outputFormat->flags & AVFMT_NOFILE)) {
            if (_liveStream.enabled) {
                if (avio_open(&avFormat->pb, "/Users/josh04/WebRoot/stream.m3u8", AVIO_FLAG_WRITE) < 0) {
                    throw std::runtime_error("FFMPEG: Stream output open failed.");
                }
            } else {
                if (avio_open(&avFormat->pb, fullPath.c_str(), AVIO_FLAG_WRITE) < 0) {
                    throw std::runtime_error("FFMPEG: File output open failed.");
                }
            }
        }
        
        if (avformat_write_header(avFormat.get(), NULL)) {
            throw std::runtime_error("FFMPEG: Failed to write header.");
        }
        
        //bool incoming = true;
        //bool running = true;
        int i = 0;
        while (running(runnings)) {
            for (int j = 0; j < _encoders.size(); ++j) {
                if (!_encoders[j]->good()) {
                    runnings[j] = false;
                    if (j == 0) {
//                       std::dynamic_pointer_cast<aacEncoder>(_encoders[_encoders.size()-1])->breakTrigger();
                    }
                } else {
                    auto buf = _encoders[j]->outLock(); // 30 nanosecond wait for packet
                    if (buf != nullptr) {
						AVPacket * packet = (AVPacket *)buf.get_pointer();
//                        if (packet->data != NULL) {
                            /* rescale output packet timestamp values from codec to stream timebase */
                        AVRational time_base;
                        auto ptr = _encoders[j]->libavformatContext();
                        if (ptr != nullptr) {
                            time_base = ptr->time_base;
                        }
                        
                            packet->pts = av_rescale_q_rnd(packet->pts, time_base, videoStreams[j]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                            packet->dts = av_rescale_q_rnd(packet->dts, time_base, videoStreams[j]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
/*                            if (packet->pts == 0) {
                                packet->pts = last_pts;
                                packet->dts = last_pts;
                            } else {
                                last_pts = packet->pts;
                            }*/
                            packet->duration = av_rescale_q(packet->duration, time_base, videoStreams[j]->time_base);
                            packet->stream_index = videoStreams[j]->index;
                            
                            /* Write the compressed frame to the media file. */
            #ifdef _VIDEO_MUSH_FFMPEG_DEBUG
                            std::stringstream strm;
                            strm << boost::str(boost::format("%12i ") % i) << "Packet Output";
                            putLog(strm.str().c_str());
            #endif
                            //if (j != 2) {
                            //    putLog("encoding go!");
                            //}
                        
                            av_interleaved_write_frame(avFormat.get(), packet);
                            //av_free_packet(packet);
                            ++i;
//                        }
                        if (j == 0) {
 //                           std::dynamic_pointer_cast<aacEncoder>(_encoders[_encoders.size()-1])->fireTrigger();
                        }
                        _encoders[j]->outUnlock();
                    }
                }
            }
        }

        av_write_trailer(avFormat.get());
		avio_close(avFormat->pb);
        _encoders.clear();
        avFormat = nullptr;
    }
    
private:
    bool running(const std::vector<bool> &runnings) {
        bool ret = false;
        for (auto run : runnings) {
            ret = ret || run;
        }
        return ret;
    }

    std::vector<shared_ptr<encoderEngine>> _encoders;
    const char * filename = nullptr, * path = nullptr;
    
    mush::core::outputConfigStruct::liveStreamStruct _liveStream;
    
    unsigned int _width, _height;
    int64_t last_pts = 0;
    
    const char * container_string = "";
    
    std::shared_ptr<AVFormatContext> avFormat = nullptr;
    
};

#endif
