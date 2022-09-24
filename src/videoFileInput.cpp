//
//  videoFileInput.cpp
//  video-mush
//
//  Created by Josh McNamee on 20/10/2016.
//
//

#include <sstream>
#include "videoFileInput.hpp"

VideoFileInput::VideoFileInput(bool isDummyVideo) : mush::scrubbableFrameGrabber(mush::inputEngine::videoInput), ffmpegWrapper() {
    setTagInGuiName("New FFmpeg Decoder");
}

VideoFileInput::~VideoFileInput() {
    
}

void VideoFileInput::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    avFormatContext = avformat_alloc_context();
    _streamCount = 2;
    if (avformat_open_input(&avFormatContext, config.inputPath, nullptr, nullptr) != 0) {
        throw std::runtime_error("Error while calling avformat_open_input (probably invalid file format)");
    }
    
    if (avformat_find_stream_info(avFormatContext, nullptr) < 0) {
        throw std::runtime_error("Error while calling avformat_find_stream_info");
    }
    
    for (unsigned int i = 0; i < avFormatContext->nb_streams; ++i) {
        auto istream = avFormatContext->streams[i];		// pointer to a structure describing the stream
        auto codecType = istream->codec->codec_type;	// the type of data in this stream, notable values are AVMEDIA_TYPE_VIDEO and AVMEDIA_TYPE_AUDIO
        if (codecType == AVMEDIA_TYPE_VIDEO) {
            if (lumaStream == nullptr) {
                lumaStream = istream;
            } else {
                chromaStream = istream;
            }
        }
    }
    
    if (!lumaStream) {
        throw std::runtime_error("Didn't find video streams in the file (probably audio file)");
    }
    
    if (!chromaStream) {
        _streamCount = 1;
    }
    
    // getting the required codec structure
    const auto codecL = avcodec_find_decoder(lumaStream->codec->codec_id);
    if (codecL == nullptr)
        throw std::runtime_error("Codec required by video file not available");
    
    // allocating a structure
    avLumaCodecContext = avcodec_alloc_context3(codecL);
    
    avLumaCodecContext->extradata = lumaStream->codec->extradata;
    avLumaCodecContext->extradata_size = lumaStream->codec->extradata_size;
    
    
    // getting the required codec structure
    AVCodec * codecC;
    if (_streamCount == 2) {
        codecC = avcodec_find_decoder(chromaStream->codec->codec_id);
        if (codecC == nullptr)
            throw std::runtime_error("Codec required by video file not available");
        
        // allocating a structure
        avChromaCodecContext = avcodec_alloc_context3(codecC);
        
        avChromaCodecContext->extradata = chromaStream->codec->extradata;
        avChromaCodecContext->extradata_size = chromaStream->codec->extradata_size;
    }
    // initializing the structure by opening the codec
    if (avcodec_open2(avLumaCodecContext, codecL, nullptr) < 0) {
        throw std::runtime_error("Could not open codec");
    }
    if (_streamCount == 2) {
        if (avcodec_open2(avChromaCodecContext, codecC, nullptr) < 0) {
            throw std::runtime_error("Could not open codec");
        }
        
        _width = chromaStream->codec->width;
        _height = chromaStream->codec->height;
        _frameCount = chromaStream->nb_frames + 1 - ((chromaStream->nb_frames + 1) % 2);
    } else {
        
        _width = lumaStream->codec->width;
        if (config.doubleHeightFrame) {
            doubleHeightFrame = true;
            _height = lumaStream->codec->height/2;
        } else {
            _height = lumaStream->codec->height;
        }
        _frameCount = lumaStream->nb_frames + 1 - ((lumaStream->nb_frames + 1) % 2);
    }
    
    
    
    if (_width * _height > 0) {
        for (int i = 0; i < config.inputBuffers*config.exposures; i++) {
            addItem(context->hostWriteBuffer(_width * _height * 4 * 2));
        }
        
    }
    
    _loop = config.loopFrames;
    
}

void VideoFileInput::getDetails(mush::core::inputConfigStruct &config) {
}

void VideoFileInput::gather() {
    SetThreadName("NewVideoDecoder");
    // data in the packet of data already processed
    size_t offsetInData = 0;
    // let's read some data
    
    // allocating an AVFrame
    
    struct SwsContext* convertCtx;
    
    if (_streamCount == 1 && !doubleHeightFrame) {
        convertCtx = sws_getContext(_width, _height, lumaStream->codec->pix_fmt, _width, _height, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
    } else {
        convertCtx = sws_getContext(_width, _height*2, lumaStream->codec->pix_fmt, _width, _height*2, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
    }
    
    AVPacket packet;
    memset(&packet, 0, sizeof(AVPacket));
    bool chromaDone = false;
    bool lumaDone = false;
    uint8_t * ptr = nullptr;
    _currentFrame = 0;
    int p = 0;
    bool run = true;
    while (run && (_currentFrame < _frameCount || _frameCount == 0)) {
        // Wait for new frame
        std::shared_ptr<AVFrame> avFrame(av_frame_alloc(), [](AVFrame* a){ av_frame_free(&a); });
        bool runningOnFumes = false;
        // the decoding loop, running until EOF
        if (offsetInData >= packet.size) {
            bool waiting = true;
            while (waiting) {
                
                {
                    std::lock_guard<std::mutex> lock(_scrubMutex);
                    if (_scrub) {
                        if (_scrubTo < _frameCount) {
                            //av_seek_frame(avFormatContext, 0, _scrubTo, 0);
                            
                            avformat_seek_file(avFormatContext, -1, INT64_MIN, _scrubTo, INT64_MAX, 0);
                            
                            avcodec_flush_buffers(avLumaCodecContext);
                            
                            _currentFrame = _scrubTo;
                        }
                    }
                    _scrub = false;
                }
                
                if (av_read_frame(avFormatContext, &packet) < 0) {
                    packet.data = nullptr;
                    packet.size = 0;
                    packet.dts = INT64_MAX;
                    if (p || _streamCount == 2) {
                        packet.stream_index = chromaStream->index;
                    } else {
                        packet.stream_index = lumaStream->index;
                    }
                    runningOnFumes = true;
                    if (_streamCount == 2) {
                        p = (p + 1) % 2;
                    }
                    //running = false;		// we are at EOF
                }
                if (packet.stream_index == lumaStream->index) {
                    waiting = false;
                }
                
                if (_streamCount == 2) {
                    if (packet.stream_index == chromaStream->index) {
                        waiting = false;
                    }
                }
                
                
                if (first_dts == INT64_MAX) {
                    first_dts = packet.dts;
                    first_stream = packet.stream_index;
                }
                
                if (waiting == true) {
                    av_free_packet(&packet);
                }
            }
            offsetInData = 0;
        }
        
        /*
        std::stringstream strm;
        
        strm << "DTS: " << packet.dts;
        putLog(strm.str());
         */
        
        // preparing the packet that we will send to libavcodec
        AVPacket packetToSend;
        memset(&packetToSend, 0, sizeof(AVPacket));
        //memcpy(&packetToSend, &packet, sizeof(AVPacket));
        packetToSend.data = packet.data + offsetInData;
        packetToSend.size = packet.size - offsetInData;
        
        packetToSend.side_data = packet.side_data;
        packetToSend.side_data_elems = packet.side_data_elems;
        
        packetToSend.dts = packet.dts;
        
        avLumaCodecContext->extradata = lumaStream->codec->extradata;
        avLumaCodecContext->extradata_size = lumaStream->codec->extradata_size;
        
        int isFrameAvailable = 0;
        int64_t processedLength = 0;
        // sending data to libavcodec
        if (packet.stream_index == lumaStream->index) {
            processedLength = avcodec_decode_video2(avLumaCodecContext, avFrame.get(), &isFrameAvailable, &packetToSend);
        } else if (_streamCount == 2) {
            if (packet.stream_index == chromaStream->index) {
                processedLength = avcodec_decode_video2(avChromaCodecContext, avFrame.get(), &isFrameAvailable, &packetToSend);
            }
        }
        
        if (processedLength < 0) {
            av_free_packet(&packet);
            throw std::runtime_error("Error while processing the data");
        }
        
        offsetInData += processedLength;
        
        // processing the image if available
        if (isFrameAvailable) {
            int dststride = _width * 4;
            
            if (ptr == nullptr) {
                auto buf = inLock();
                if (buf == nullptr) {
                    release();
                    return;
                }
				ptr = (uint8_t *)buf.get_pointer();
            }
            if (packet.stream_index == lumaStream->index || _streamCount == 1) {
                if (lumaDone) {
                    putLog("WARNING- DROPPED A LUMA FRAME");
                    //						lumaDone = false;
                }
                
                //lumaDone = true;
                if (chromaDone) {
                    if (chromaId == avFrame->coded_picture_number) {
                        sws_scale(convertCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, &ptr, &dststride);
                        //std::cout << "Luma: " << avFrame->coded_picture_number << std::endl;
                        lumaDone = true;
                    } else if (chromaId < avFrame->coded_picture_number) {
                        sws_scale(convertCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, &ptr, &dststride);
                        //std::cout << "Luma: " << avFrame->coded_picture_number << std::endl;
                        lumaDone = true;
                        chromaDone = false;
                        lumaId = avFrame->coded_picture_number;
                    } else {
                        //lumaDone = false;
                    }
                } else {
                    lumaId = avFrame->coded_picture_number;
                    sws_scale(convertCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, &ptr, &dststride);
                    //std::cout << "Luma: " << avFrame->coded_picture_number << std::endl;
                    lumaDone = true;
                }
                //sws_scale(convertCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, &ptr, &dststride);
                
            } else {
                
                if (chromaDone) {
                    putLog("WARNING- DROPPED A CHROMA FRAME");
                    //						chromaDone = false;
                }
                
                if (lumaDone) {
                    if (lumaId == avFrame->coded_picture_number) {
                        unsigned char * ptr2 = ptr + _width*_height * 4;
                        //std::cout << "Chroma: " << avFrame->coded_picture_number << std::endl;
                        sws_scale(convertCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, &ptr2, &dststride);
                        chromaDone = true;
                        lumaDone = true;
                    } else if (lumaId < avFrame->coded_picture_number) {
                        unsigned char * ptr2 = ptr + _width*_height * 4;
                        //std::cout << "Chroma: " << avFrame->coded_picture_number << std::endl;
                        sws_scale(convertCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, &ptr2, &dststride);
                        chromaDone = true;
                        lumaDone = false;
                        chromaId = avFrame->coded_picture_number;
                    } else {
                        //chromaDone = false;
                    }
                } else {
                    chromaId = avFrame->coded_picture_number; 
                    unsigned char * ptr2 = ptr + _width*_height * 4;
                    //std::cout << "Chroma: " << avFrame->coded_picture_number << std::endl;
                    sws_scale(convertCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, &ptr2, &dststride);
                    chromaDone = true;
                }
                
                /*unsigned char * ptr2 = ptr + _width*_height * 4;
                 //std::cout << "Chroma: " << avFrame->coded_picture_number << std::endl;
                 sws_scale(convertCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, &ptr2, &dststride);
                 chromaDone = true;*/
            }
            
            
            if ((lumaDone && chromaDone) || _streamCount == 1) {
                lumaDone = false;
                chromaDone = false;
                ptr = nullptr;
                inUnlock();
                ++_currentFrame;
            }
        } else if (runningOnFumes) {
            if (_loop) {
                moveToFrame(0);
            } else {
                run = false;
            }
        }
        if (_loop) {
            if (_frameCount <= _currentFrame) {
                _currentFrame = _frameCount - 1; // hacky hack hack
                moveToFrame(0);
                /*
                std::stringstream strm;
                
                strm << "Flushed";
                putLog(strm.str());
                */
            }
        }
    }
    release();
    
    avcodec_close(avLumaCodecContext);
    av_free(avLumaCodecContext);
    avcodec_close(avChromaCodecContext);
    av_free(avChromaCodecContext);
    avformat_free_context(avFormatContext);
}


void VideoFileInput::moveToFrame(int frame) {
    std::lock_guard<std::mutex> lock(_scrubMutex);
    _scrub = true;
    _scrubTo = frame;
}

int VideoFileInput::getFrameCount() {
    return _frameCount;
}

int VideoFileInput::getCurrentFrame() {
    return _currentFrame;
}
