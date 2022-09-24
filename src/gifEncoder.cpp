//
//  gifEncoder.cpp
//  video-mush
//
//  Created by Josh McNamee on 25/09/2016.
//
//

#include <sstream>
#include <boost/format.hpp>
#include <gif_lib.h>

#include <Mush Core/opencl.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/checkExists.hpp>
#include "gifEncoder.hpp"

gifEncoder::gifEncoder(unsigned int i) : encoderEngine(), _stream_number(i) {
    
}

gifEncoder::~gifEncoder() {
    
}

void gifEncoder::init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) {
    
    queue = context->getQueue();
    _input = castToImage(outBuffer);
    _token = _input->takeFrameToken();
    
    unsigned int size;
    _input->getParams(_inputWidth, _inputHeight, size);
    _outputWidth = _inputWidth;
    _outputHeight = _inputHeight;
    // geefEncoder outputs files, but still needs to be governed by a nullEncoder
    addItem(&done);
    
    filename = config.outputName;
    path = config.outputPath;
    
    _downsample = context->intImage(_outputWidth, _outputHeight);
    _copy_image = context->getKernel("encodeSRGB");
    _host_buffer = context->hostWriteBuffer(_outputWidth * _outputHeight * sizeof(uint8_t) * 4);
    
    _fps = config.fps;
}

void gifEncoder::gather() {
    int64_t counter = 0;
    
    
    std::stringstream strm;
    strm << std::string(path) << "/" << std::string(filename) << "_stream_" << _stream_number << ".gif";
    
	std::string final = mush::checkExists(strm.str());

    auto gif = start_gif(final.c_str());
    
    bool running = true;
    
    while (running) {
        
        auto ptr = _input->outLock(0, _token);
        if (ptr == nullptr && _token == -1) {
            running = false;
            break;
        }
        
        _copy_image->setArg(0, ptr.get_image());
        _copy_image->setArg(1, *_downsample);
        
        cl::Event event;
        queue->enqueueNDRangeKernel(*_copy_image, cl::NullRange, cl::NDRange(_outputWidth, _outputHeight), cl::NullRange, NULL, &event);
        
        event.wait();
        
        _input->outUnlock();
        
        cl::size_t<3> origin;
        cl::size_t<3> region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _outputWidth; region[1] = _outputHeight; region[2] = 1;
        
        queue->enqueueReadImage(*_downsample, CL_TRUE, origin, region, 0, 0, _host_buffer, NULL, &event);
        event.wait();
    
        add_gif_frame(gif, _host_buffer);
        
        inLock();
        inUnlock();
        
        ++counter;
    }
    
    close_gif(gif);
    release();
}

GifFileType * gifEncoder::start_gif(const char * output_path) {
    int error = 0;
    GifFileType * gif_file = EGifOpenFileName(output_path, FALSE, &error);
    
    return gif_file;
}

void gifEncoder::add_gif_frame(GifFileType * file, uint8_t *data) {
    
    std::vector<uint8_t>    output(_outputWidth * _outputHeight);
    
    if (!_wrote_first_frame) {
        
        
        std::vector<uint8_t>    r(_outputWidth * _outputHeight),
                                g(_outputWidth * _outputHeight),
                                b(_outputWidth * _outputHeight);
        
        
        for (int j = 0; j < _outputHeight; ++j) {
            for (int i = 0; i < _outputWidth; ++i) {
                r[j*_outputWidth + i] = data[(j * _outputWidth + i) * 4];
                g[j*_outputWidth + i] = data[(j * _outputWidth + i) * 4 + 1];
                b[j*_outputWidth + i] = data[(j * _outputWidth + i) * 4 + 2];
            }
        }
        
        int paletteSize = 256;
        
        auto outputPalette = GifMakeMapObject(paletteSize, NULL);
        
        //if (!outputPalette) return false;
        
        GifQuantizeBuffer(_outputWidth, _outputHeight, &paletteSize, r.data(), g.data(), b.data(), output.data(), outputPalette->Colors);
        
        EGifPutScreenDesc(file, _outputWidth, _outputHeight, 8, 0, outputPalette);
        GifFreeMapObject(outputPalette);
        int loop_count = 0;
        {
            char nsle[12] = "NETSCAPE2.0";
            char subblock[3];
            
            EGifPutExtensionLeader(file, APPLICATION_EXT_FUNC_CODE);
            EGifPutExtensionBlock(file, 11, nsle);
            //EGifPutExtensionFirst(file, APPLICATION_EXT_FUNC_CODE, 11, nsle);
            
            subblock[0] = 1;
            subblock[2] = loop_count % 256;
            subblock[1] = loop_count / 256;
            
            EGifPutExtensionBlock(file, 3, subblock);
            EGifPutExtensionTrailer(file);
            
            //EGifPutExtensionLast(file, APPLICATION_EXT_FUNC_CODE, 3, subblock);
            
        }
        
        _wrote_first_frame = true;
    } else {
        
        for (int j = 0; j < _outputHeight; ++j) {
            for (int i = 0; i < _outputWidth; ++i) {
                int minIndex = 0,
                minDist = 3 * 256 * 256;
                GifColorType * c = file->SColorMap->Colors;
                
                /* Find closest color in first color map to this color. */
                for (int k = 0; k < file->SColorMap->ColorCount; k++) {
                    int dr = (int(c[k].Red) - data[(j * _outputWidth + i) * 4] ) ;
                    int dg = (int(c[k].Green) - data[(j * _outputWidth + i) * 4+1] ) ;
                    int db = (int(c[k].Blue) - data[(j * _outputWidth + i) * 4+2] ) ;
                    
                    int dist=dr*dr+dg*dg+db*db;
                    
                    if (minDist > dist) {
                        minDist  = dist;
                        minIndex = k;
                    }
                }
                
                output[j * _outputWidth + i] = minIndex;
            }
        }
        
    }
    
    int delay = 100.0f / _fps;
    unsigned char ExtStr[4] = { 0x04, 0x00, 0x00, 0xff };
    
    
    ExtStr[0] = (false) ? 0x06 : 0x04;
    ExtStr[1] = delay % 256;
    ExtStr[2] = delay / 256;
    
    /* Dump graphics control block. */
    EGifPutExtension(file, GRAPHICS_EXT_FUNC_CODE, 4, ExtStr);
    
    
    EGifPutImageDesc(file, 0, 0, _outputWidth, _outputHeight, FALSE, NULL);
    
    for (int j = 0; j < _outputHeight; ++j) {
        EGifPutLine(file, &output[j * _outputWidth], _outputWidth);
    }
}

void gifEncoder::close_gif(GifFileType * file) {
    int error = 0;
    EGifCloseFile(file, &error);
}
