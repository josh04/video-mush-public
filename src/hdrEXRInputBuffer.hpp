//
//  hdrEXRInputBuffer.hpp
//  video-mush
//
//  Created by Visualisation on 11/03/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_hdrEXRInputBuffer_hpp
#define media_encoder_hdrEXRInputBuffer_hpp

#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <Mush Core/frameGrabber.hpp>
#include <Mush Core/hdrEXR.hpp>

using namespace boost::filesystem;

class hdrEXRInputBuffer : public mush::frameGrabber {
public:
	hdrEXRInputBuffer() : mush::frameGrabber(mush::inputEngine::fastEXRInput), work(ioService) {
		
	}
	
	~hdrEXRInputBuffer() {}
	
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		
		this->input = std::string(config.inputPath);
		_size = 0;
		_width = 0;
		_height = 0;
		_exposures = config.exposures;
		
		std::string path;
		
		directory_iterator itr(input);
		std::string ext;
		bool found = false;
		directory_iterator end_itr;
		
		while ((is_directory(itr->path()) || !found) && itr != end_itr) {
			++itr;
			ext = itr->path().extension().string();
			if (ext == ".exr") {
				found = true;
			}
		}
		
		path = itr->path().string();
		std::ifstream in(path.c_str(), std::ios_base::in | std::ios_base::binary);
		_size = (int)sizeof(half); // FIXME
		hdrEXR::ReadSize(path.c_str(), _width, _height);
		
		in.close();
				
		for (int i = 0; i < 4; i++) {
			addItem(context->hostWriteBuffer(_width*_height*4*sizeof(half)));
			threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
            taken.push_back(false);
		}
        
	}
    
    void getDetails(mush::core::inputConfigStruct &config) override {
		config.exposures = _exposures;
	}
	
	void gather() {
		directory_iterator end_itr;
		
		unsigned char * ptr;
		
		std::string ext, altext;
		
		ext = ".exr"; altext = ".exr";
		std::vector<path> paths;
		
		paths.empty();
		for ( directory_iterator itr( input ); itr != end_itr; ++itr ) {
			paths.push_back(itr->path());
		}
		sort(paths.begin(), paths.end());
		_numberOfFrames = paths.size();
		//		for (int q = 0; q < 60; ++q) {
		
        std::mutex mut;
        
		for (std::vector<path>::iterator it = paths.begin(); it != paths.end(); ++it) {
			if (it->extension().string() == ext || it->extension().string() == altext) {
                std::unique_lock<std::mutex> unique(mut);
                cond.wait(unique, [&](){return !taken[next];});
                          
				auto buf = inLock();
				
				if (buf == nullptr) {
					return;
				}

				taken[next] = true;
				ioService.post(boost::bind(&hdrEXRInputBuffer::readEXR, this, buf, it->string().c_str(), next));
				inUnlock();


			}
		}
		//		}
        for (int i; i < taken.size(); ++i) {
            std::unique_lock<std::mutex> unique(mut);
            cond.wait(unique, [&](){return !taken[next];});
        }
		release();
	}
	
    void readEXR(mush::buffer& buf, const char * path, int unlock) {
		hdrEXR::ReadEXR(buf, path);
		inUnlock(unlock);
	}
	
protected:
	
	virtual void inUnlock(int unlock) {
		empty[unlock] = false;
        taken[unlock] = false;
        cond.notify_all();
		outConds[unlock]->notify_one();
	}
	
	virtual void inUnlock() {
		next = (next + 1) % getBuffers();
	}
	
private:
	
	std::string input;
	int _exposures;
	boost::asio::io_service ioService;
	boost::thread_group threadpool;
    boost::asio::io_service::work work;
    std::condition_variable cond;
    
    std::vector<bool> taken;
};

#endif
