#ifndef EXRENCODER_HPP
#define EXRENCODER_HPP

#include <sstream>

#include <boost/date_time.hpp>

#include <Mush Core/hdrEXR.hpp>
#include <Mush Core/ringBuffer.hpp>
#include <Mush Core/encoderEngine.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>


class exrEncoder : public encoderEngine {
public:
	exrEncoder(unsigned int i) : encoderEngine(), streamNo(i) {
		
		counter = 0;
	}

	~exrEncoder() {
	}
    
    virtual void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) {
		counter = config.count_from;
        queue = context->getQueue();
        imageBuf = castToImage(outBuffer);
        
        unsigned int size;
        imageBuf->getParams(_inputWidth, _inputHeight, size);
        _outputWidth = _inputWidth;
        _outputHeight = _inputHeight;
        // exrEncoder outputs files, but still needs to be governed by a nullEncoder
        addItem(mush::buffer(&done));
        
		toEXR.init(context);
        filename = config.outputName;
        path = config.outputPath;
        _timestamp = config.exr_timestamp;
        
        _token = imageBuf->takeFrameToken();
    }
    
    virtual std::shared_ptr<AVCodecContext> libavformatContext() { return nullptr; }
    
protected:
	void gather() {
        
        //int64_t counter = 0;
        bool running = true;
        while (running) {
            

			std::stringstream folder;
			folder << std::string(path) << "/" << streamNo;

			boost::filesystem::path dir(folder.str());
			if (boost::filesystem::create_directory(dir)) {
				putLog(std::string("Directory Created: ") + folder.str());
			}

            std::stringstream stream;
            stream << std::string(path) << "/" << streamNo << "/" << std::string(filename);
            
            if (_timestamp) {
                
                char buffer[80];
                
                std::tm t = boost::posix_time::to_tm(boost::posix_time::second_clock::local_time());
                
                strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S", &t);
                
                stream << "_" << buffer;
            }
            
            stream << "_StreamNo_" << streamNo << "_" << boost::str(boost::format("%05d") % counter) << ".exr";

            auto ptr = imageBuf->outLock(0, _token);
            if (ptr == nullptr) {
                running = false;
                break;
            }
        
            toEXR.write(ptr, stream.str(), _outputWidth, _outputHeight);
            imageBuf->outUnlock();
            
            inLock();
            inUnlock();
            
            ++counter;
        }
        release();
	}

private:
    const bool done = true;
    
	cl::Event event;
	cl::CommandQueue * queue = nullptr;
    
    mush::registerContainer<mush::imageBuffer> imageBuf;
	hdrEXR::clToEXR toEXR;
	const char * filename = nullptr;
	const char * path = nullptr;
	int64_t counter = 0;
    unsigned int streamNo = 0;
    size_t _token = -1;
    
    bool _timestamp = false;
};

#endif
