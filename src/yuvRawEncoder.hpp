//
//  yuv12bitEncoder.hpp
//  video-mush
//
//  Created by Josh McNamee on 08/01/2015.
//
//

#ifndef video_mush_yuv12bitEncoder_hpp
#define video_mush_yuv12bitEncoder_hpp

#include <sstream>
#include <fstream>
#include <Mush Core/ringBuffer.hpp>
#include "yuvDepthClampProcess.hpp"

#include <Mush Core/encoderEngine.hpp>

namespace mush {
    class yuvRawEncoder : public encoderEngine {
    public:
        yuvRawEncoder(unsigned int i) : encoderEngine(), streamNo(i) {
            
            counter = 0;
        }
        
        ~yuvRawEncoder() {
        }
        
        virtual void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) {
            
            queue = context->getQueue();
            
            _clamp = std::make_shared<mush::yuvDepthClampProcess>(12, 1.0f, transfer::rec709);
            _clamp->init(context, outBuffer);
            
            unsigned int size;
            _clamp->getParams(_inputWidth, _inputHeight, size);
            _outputWidth = _inputWidth;
            _outputHeight = _inputHeight;
            
            _host_buffer = (unsigned char *)context->hostWriteBuffer(_inputWidth*_inputHeight*sizeof(uint16_t)*2);
            
            // exrEncoder outputs files, but still needs to be governed by a nullEncoder
            addItem(&done);
            
            filename = config.outputName;
            path = config.outputPath;
            
        }
        
        virtual std::shared_ptr<AVCodecContext> libavformatContext() { return nullptr; }
    
    protected:
        void gather() {
            
            
            int64_t counter = 0;
            bool running = true;
        
            std::stringstream stream;
            stream << std::string(path) << "/" << std::string(filename) << "_StreamNo_" << streamNo << "_" << boost::str(boost::format("%02d") % counter) << ".yuv";
            
            std::ofstream outStream;
            
            outStream = std::ofstream(stream.str().c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            
            
            while (running) {
                
                _clamp->process();
                
                auto ptr = _clamp->outLock();
                if (ptr == nullptr) {
                    running = false;
                    break;
                }
                
                cl::Event event;
                queue->enqueueReadBuffer(ptr.get_buffer(), CL_TRUE, 0, _inputWidth*_inputHeight*sizeof(uint16_t)*2, _host_buffer, NULL, &event);
                event.wait();
                
                outStream.write((const char *)_host_buffer, _outputWidth*_outputHeight*2*sizeof(uint16_t));
                
            
                _clamp->outUnlock();
                
                
                inLock();
                inUnlock();
                
                ++counter;
            }
            
            outStream.close();
                
            release();
            
        }
        
    private:
        const bool done = true;
        
        cl::Event event;
        cl::CommandQueue * queue = nullptr;
        
        unsigned char * _host_buffer = nullptr;
        
        const char * filename = nullptr;
        const char * path = nullptr;
        int counter = 0;
        unsigned int streamNo = 0;
        
        std::shared_ptr<yuvDepthClampProcess> _clamp = nullptr;
    };
    
}

#endif
