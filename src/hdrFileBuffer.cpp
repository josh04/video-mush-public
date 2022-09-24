//
//  hdrFileBuffer.cpp
//  video-mush
//
//  Created by Visualisation on 08/03/2014.
//  Copyright (c) 2014. All rights reserved.
//

#define _I_EXR
#define _I_TIFF


extern void SetThreadName(const char* threadName);

#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#ifdef _I_TIFF
#include <tiffio.h>
#endif

#include <Mush Core/opencl.hpp>
#include "hdrPFM.hpp"
#include <Mush Core/hdrEXR.hpp>
#include <Mush Core/pipeio.hpp>

#include <Mush Core/frameGrabber.hpp>
#include "hdrFileBuffer.hpp"

using std::shared_ptr;
using std::make_shared;
using namespace boost::filesystem;

void hdrFileBuffer::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
	_filetype = config.filetype;
	_pipe = false;
	this->input = config.inputPath;
	_size = 0;
	_width = 0;
	_height = 0;
	_tiffFloat = false;
    
    _loopFrames = config.loopFrames;
	
	std::string path;
	
	if (!_pipe){
		
		directory_iterator itr(input);
		std::string ext;
		bool found = false;
        directory_iterator end_itr;
		
		while (itr != end_itr && (is_directory(itr->path()) || !found)) {
            if (itr->path().string()[0] == '.') {
                continue;
            }
			found = true;
			ext = itr->path().extension().string();
			if (_filetype == mush::filetype::detectFiletype) {
				if (ext == ".pfm") {
					_filetype = mush::filetype::pfmFiletype;
#ifdef _I_EXR
				} else if (ext == ".exr") {
					_filetype = mush::filetype::exrFiletype;
#endif
				} else if (ext == ".raw") {
					_filetype = mush::filetype::rawFiletype;
#ifdef _I_TIFF
				} else if (ext == ".tiff" || ext == ".tif") {
					_filetype = mush::filetype::mergeTiffFiletype;
#endif
				} else {
					found = false;
				}
			} else if (_filetype == mush::filetype::pfmFiletype) {
				if (ext != ".pfm") {
					found = false;
				}
#ifdef _I_EXR
			} else if (_filetype == mush::filetype::exrFiletype) {
				if (ext != ".exr") {
					found = false;
				}
#endif
			} else if (_filetype == mush::filetype::rawFiletype) {
				if (ext != ".raw") {
					found = false;
				}
#ifdef _I_TIFF
			} else if (_filetype == mush::filetype::mergeTiffFiletype) {
				if (ext != ".tiff" || ext == ".tif") {
					found = false;
				}
#endif
			} else {
				found = false;
			}
            if (found == false) {
                ++itr;
            }
		}
        
        if (found == false) {
            throw std::runtime_error("Image input: No supported image files found.");
        }
		
		path = itr->path().string();
		std::ifstream in(path.c_str(), std::ios_base::in | std::ios_base::binary);
#ifdef _I_TIFF
		TIFF* tiff;
		short tiffSampleFormat;
#endif
		
		switch(_filetype) {
			case mush::filetype::pfmFiletype:
			_size = (int)sizeof(float);
			hdrPFM::readPFMHeader(in, _width, _height);
			break;
#ifdef _I_EXR
			case mush::filetype::exrFiletype:
			Imf::setGlobalThreadCount(std::thread::hardware_concurrency() - 1);
			_size = (int)sizeof(half); // FIXME
			hdrEXR::ReadSize(path.c_str(), _width, _height);
			break;
#endif
			case mush::filetype::rawFiletype:
			_size = (int)sizeof(float);
			_width = config.inputWidth;
			_height = config.inputHeight;
			break;
#ifdef _I_TIFF
			case mush::filetype::mergeTiffFiletype:
			_size = (int)sizeof(uint8_t);
			tiff = TIFFOpen(path.c_str(), "r");
			
			TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &_width);
			TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &_height);
			TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &tiffSampleFormat);
			
			if (tiffSampleFormat == 32) {
				_size = (int)sizeof(float);
				_filetype = mush::filetype::tiffFiletype;
			}
			
			TIFFClose(tiff);
			break;
#endif
			default:
			throw std::exception();
		}
		
		in.close();
	} else {
		_size = (int)sizeof(float);
		_width = config.inputWidth;
		_height = config.inputHeight;
	}
    
	for (int i = 0; i < config.inputBuffers*config.exposures; i++) {
		addItem(context->hostWriteBuffer(_width*_height*4*_size));
	}
    
	
}

void hdrFileBuffer::getDetails(mush::core::inputConfigStruct &config) {
	config.filetype = _filetype;
}

void hdrFileBuffer::gather() {
	
	SetThreadName("folderInput");
	
	directory_iterator end_itr;
	
	unsigned char * ptr;
	
	std::string ext, altext;
	
	switch(_filetype) {
		case mush::filetype::pfmFiletype:
		ext = ".pfm"; altext = ".pfm";
		break;
#ifdef _I_EXR
		case mush::filetype::exrFiletype:
		ext = ".exr"; altext = ".exr";
		break;
#endif
		case mush::filetype::rawFiletype:
		ext = ".raw"; altext = ".raw";
		break;
#ifdef _I_TIFF
		case mush::filetype::mergeTiffFiletype:
		ext = ".tif"; altext = ".tiff";
		break;
#endif
        default:
            return;
	}
	
	std::string pat;
	std::vector<path> paths;
	
	if (_pipe) {
		std::ifstream in(input.c_str(), std::ios_base::in | std::ios_base::binary);
		Imf::PipeInStream im(in, (const char*)"(input.pipe)");
		while (in.good()) {
			/*for (int j = 0; j < buffers ; ++j) {
			 cout << " " << (*locks)[j];
			 }
			 cout << endl;*/

			auto buf = inLock();
			if (buf == nullptr) {
				return;
			}
			unsigned char * ptr = (unsigned char *)buf.get_pointer();
			if (_filetype == mush::filetype::pfmFiletype) {
				hdrPFM::readPFM(in, ptr);
#ifdef _I_EXR
			} else if (_filetype == mush::filetype::exrFiletype) {
                hdrEXR::ReadEXR(ptr, im);
#endif
#ifdef _I_TIFF
			} else if (_filetype == mush::filetype::tiffFiletype) {
				//TIFF* tiff;
#endif
			} else {
				hdrPFM::readFromPipe(in, reinterpret_cast<char*>(ptr), sizeof(float) * _width * _height * 4);
			}
			
			inUnlock();
			
		}
		in.close();
		
	} else {
		int cnt = std::distance(directory_iterator( input ), directory_iterator());

		paths.empty();
		paths.reserve(cnt);
		for ( directory_iterator itr( input ); itr != end_itr; ++itr ) {
            if (itr->path().filename().string()[0] != '.') {
                paths.push_back(itr->path());
            }
		}
		sort(paths.begin(), paths.end());
		//_numberOfFrames = paths.size();
		//		for (int q = 0; q < 60; ++q) {
        
        _frameCount = 0;
        for (std::vector<path>::iterator it = paths.begin(); it != paths.end(); ++it) {
            
            if (it->extension().string() == ext || it->extension().string() == altext) {
                ++_frameCount;
            }
            
        }
    
    infinite_loop:
        _currentFrame = 0;
        uint32_t i = 0;
		for (std::vector<path>::iterator it = paths.begin(); it != paths.end(); ++it) {
            
            {
                std::lock_guard<std::mutex> lock(_scrubMutex);
                if (_scrub) {
                    if (_scrubTo < paths.size()) {
                    it = paths.begin() + _scrubTo;
                    if (it == paths.end()) {
                        break;
                    }
                        _currentFrame = _scrubTo;
                    }
                }
                _scrub = false;
            }
            
			if (it->extension().string() == ext || it->extension().string() == altext) {
				
				/*for (int j = 0; j < _buffers ; ++j) {
				 cout << " " << (*locks)[j];
				 }
				 cout << endl;*/
				auto buf = inLock();
				
				if (buf == nullptr) {
					return;
				}

				ptr = (unsigned char *)buf.get_pointer();
				
				pat = it->string();
				std::ifstream in(pat.c_str(), (std::ios_base::in | std::ios_base::binary));
				if (_filetype == mush::filetype::pfmFiletype) {
					hdrPFM::readPFM(in, ptr);
#ifdef _I_TIFF
				} else if (_filetype == mush::filetype::mergeTiffFiletype) {
					TIFFErrorHandler oldhandler;
					oldhandler = TIFFSetWarningHandler(NULL);
					TIFF* tiff = TIFFOpen(pat.c_str(), "r");
					TIFFSetWarningHandler(oldhandler);
					TIFFReadRGBAImageOriented(tiff, _width, _height, reinterpret_cast<uint32*>(ptr), 0, ORIENTATION_TOPLEFT);
					TIFFClose(tiff);
				} else if (_filetype == mush::filetype::tiffFiletype) {
					TIFFErrorHandler oldhandler;
					oldhandler = TIFFSetWarningHandler(NULL);
					TIFF* tiff = TIFFOpen(pat.c_str(), "r");
					TIFFSetWarningHandler(oldhandler);
					int strip;
					for (strip = 0; strip < TIFFNumberOfStrips(tiff); ++strip) {
						TIFFReadEncodedStrip(tiff, strip, ptr+TIFFStripSize(tiff)*strip, -1);
					}
					TIFFClose(tiff);
#endif
#ifdef _I_EXR
				} else if (_filetype == mush::filetype::exrFiletype) {
					hdrEXR::ReadEXR(buf, pat.c_str());
#endif
				} else {
					hdrPFM::readFromPipe(in, reinterpret_cast<char*>(ptr), sizeof(float) * _width * _height * 4);
				}
				in.close();
				
				inUnlock();
                ++i;
                _currentFrame++;
			}
		}
        
        if (_loopFrames) {
            goto infinite_loop;
        }
		//		}
        
	}
	release();
}


void hdrFileBuffer::moveToFrame(int frame) {
    std::lock_guard<std::mutex> lock(_scrubMutex);
    _scrub = true;
    _scrubTo = frame;
}

int hdrFileBuffer::getFrameCount() {
    return _frameCount;
}

int hdrFileBuffer::getCurrentFrame() {
    return _currentFrame;
}

