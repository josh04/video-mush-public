//
//  mlvRawInput.cpp
//  video-mush
//
//  Created by Josh McNamee on 04/08/2015.
//
//

#include <fstream>
#include <sstream>
#include <algorithm>
#include <Mush Core/opencl.hpp>

#define __attribute__(A) /* do nothing */
#include <stdint.h>
#include <raw.h>
#include <mlv.h>
#include "mlvRawInput.hpp"

#include <azure/eventkey.hpp>

using namespace mush;

#ifdef _WIN32
#define snprintf _snprintf
#define PATH_MAX MAX_PATH
#endif

extern "C" void putLog(std::string s);

mlvRawInput::mlvRawInput(int black_point, int white_point, unsigned int frame_skip) : scrubbableFrameGrabber(mush::inputEngine::mlvRawInput), azure::Eventable(), _black(black_point), _white(white_point), _frame_skip(frame_skip) {
    
}


void mlvRawInput::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    _width = config.inputWidth;
    _height = config.inputHeight;
    _size = 2;
    
    _inputs.push_back(std::make_shared<std::ifstream>(config.inputPath, std::ios_base::in | std::ios_base::binary));
    auto& input = _inputs[0];
    char ptr[5] = {"0000"};
    
    input->read(ptr, sizeof(uint8_t)*4);
    size_t cnt = input->gcount();
    
    bool foundMLVI = checkId((const char*)ptr, "MLVI");
    
    if (!foundMLVI) {
        throw std::runtime_error("Couldn't find MLVI block.");
    }

    mlv_file_hdr_t file_header;
    
    char * offset_ptr = (char *)(&file_header) + 4;
    
    input->read(offset_ptr, sizeof(mlv_file_hdr_t) - 4*sizeof(uint8_t));
    
    size_t cnt2 = input->gcount();
    
    if (input->eof() || input->fail() || input->bad() || cnt2 != sizeof(mlv_file_hdr_t) - 4*sizeof(uint8_t)) {
        throw std::runtime_error("Bad MLVI block.");
    }
    
    input->read(ptr, sizeof(uint8_t)*4);
    
    if (!checkId(ptr, "RAWI")) {
        throw std::runtime_error("Couldn't find RAWI block.");
    }
    
    mlv_rawi_hdr_t raw_header;
    offset_ptr = (char *)(&raw_header) + 4;
    input->read(offset_ptr, sizeof(mlv_rawi_hdr_t) - 4*sizeof(uint8_t));
    
    
    std::stringstream strm;
    strm << "frames: " << file_header.videoFrameCount;
    strm << " width: " << raw_header.xRes;
    strm << " height: " << raw_header.yRes << std::endl;
    strm << " depth: " << raw_header.raw_info.bits_per_pixel;
    strm << " dynamic range: " << raw_header.raw_info.dynamic_range << std::endl;
    strm << " black level: " << raw_header.raw_info.black_level << " white point: " << raw_header.raw_info.white_level;
    putLog(strm.str());
    
    _width = raw_header.xRes;
    _height = raw_header.yRes;
    _depth = raw_header.raw_info.bits_per_pixel;
	if (_black == 0) {
		_black = raw_header.raw_info.black_level;
	}
	if (_white == 0) {
		_white = raw_header.raw_info.white_level;
	}
    /*
    Imath::Matrix33<float> xyz_to_camera = {
        (float)raw_header.raw_info.color_matrix1[0]/raw_header.raw_info.color_matrix1[1],
        (float)raw_header.raw_info.color_matrix1[2]/raw_header.raw_info.color_matrix1[3],
        (float)raw_header.raw_info.color_matrix1[4]/raw_header.raw_info.color_matrix1[5],
        
        (float)raw_header.raw_info.color_matrix1[6]/raw_header.raw_info.color_matrix1[7],
        (float)raw_header.raw_info.color_matrix1[8]/raw_header.raw_info.color_matrix1[9],
        (float)raw_header.raw_info.color_matrix1[10]/raw_header.raw_info.color_matrix1[11],
        
        (float)raw_header.raw_info.color_matrix1[12]/raw_header.raw_info.color_matrix1[13],
        (float)raw_header.raw_info.color_matrix1[14]/raw_header.raw_info.color_matrix1[15],
        (float)raw_header.raw_info.color_matrix1[16]/raw_header.raw_info.color_matrix1[17],
    };
    
    _camera_to_xyz = xyz_to_camera.gjInverse();
    
    auto mat = _camera_to_xyz*xyz_to_camera;
    */
    
    
    mlv_idnt_hdr_t idnt_header;
    mlv_expo_hdr_t expo_header;
    bool idnt_found = false;
    bool expo_found = false;
    
    while (!idnt_found || !expo_found) {
        input->read(ptr, sizeof(uint8_t)*4);
        
        if (checkId(ptr, "IDNT")) {
            auto offset_ptr = (char *)(&idnt_header) + 4;
            input->read(offset_ptr, sizeof(mlv_idnt_hdr_t) - 4*sizeof(uint8_t));
            idnt_found = true;
            
            if (idnt_header.blockSize - sizeof(mlv_idnt_hdr_t) > 0) {
                input->seekg(idnt_header.blockSize - sizeof(mlv_idnt_hdr_t), std::ios::cur);
            }
            
        } else if (checkId(ptr, "EXPO")) {
            auto offset_ptr = (char *)(&expo_header) + 4;
            input->read(offset_ptr, sizeof(mlv_expo_hdr_t) - 4*sizeof(uint8_t));
            expo_found = true;
            
            if (expo_header.blockSize - sizeof(mlv_expo_hdr_t) > 0) {
                input->seekg(expo_header.blockSize - sizeof(mlv_expo_hdr_t), std::ios::cur);
            }
            
        } else if (checkId(ptr, "DISO")) {
            
            throw std::runtime_error("Hey, this file has the DISO header. Time to update the software!");
            break;
        } else {
            uint32_t blockSize;
            input->read((char*)&blockSize, sizeof(uint32_t));
            input->seekg(blockSize - 4*sizeof(uint8_t) - sizeof(uint32_t), std::ios::cur);
        }
    }
    
    if (idnt_header.cameraModel == 0x80000218) {
        _camera_type = rawCameraType::mk2;
        
        putLog("Camera Model: 5D Mk. II");
    } else if (idnt_header.cameraModel == 0x80000285) {
        _camera_type = rawCameraType::mk3;
        
        putLog("Camera Model: 5D Mk. III");
    } else {
        throw std::runtime_error("Raw camera type unsupported.");
    }
    
    //_positions.reserve(file_header.videoFrameCount);
	_mlv_raw_map = context->getKernel("mlv_raw_map");
	_upload_buffer = (uint16_t *)context->hostWriteBuffer(sizeof(uint16_t) *_width*_height * 7 / 8);
	_input_buffer = context->buffer(sizeof(uint16_t) *_width*_height * 7 / 8);

    if (_width * _height > 0) {
        for (int i = 0; i < config.inputBuffers*config.exposures; i++) {
            //addItem(context->hostWriteBuffer(_width * _height * _size * 1));
			addItem(context->buffer(_width * _height * _size * 1));
        }
    }

	queue = context->getQueue();
    _input_path = config.inputPath;
}

bool mlvRawInput::checkId(const char * ptr, const char * id) {
    if (strlen(ptr) >= 4 && strlen(id) == 4) {
        if (ptr[0] == id[0]) {
            if (ptr[1] == id[1] && ptr[2] == id[2] && ptr[3] == id[3]) {
                return true;
            }
        }
    }
    return false;
}

void mlvRawInput::gather() {
    _files_positions.push_back(std::vector<std::pair<uint64_t, std::streampos>>());
    bool running = true;
    
    while(running) {
        char magic[5];
        
        _inputs[_file_count]->read(magic, sizeof(uint8_t)*4);
        
        size_t cnt2 = _inputs[_file_count]->gcount();
        
        if (_inputs[_file_count]->eof() || _inputs[_file_count]->fail() || _inputs[_file_count]->bad() || cnt2 != 4*sizeof(uint8_t)) {
            char path[PATH_MAX];
                
            auto sec = _input_path.substr(0, _input_path.length() - 4);
                
            snprintf(path, PATH_MAX, "%s.M%02d", sec.c_str(), _file_count);
            
            _inputs.push_back(std::make_shared<std::ifstream>(path, std::ios_base::in | std::ios_base::binary));
            
            if (!(_inputs[_file_count+1]->eof() || _inputs[_file_count+1]->fail() || _inputs[_file_count+1]->bad())) {
                
                _files_positions.push_back(std::vector<std::pair<uint64_t, std::streampos>>());
                _file_count++;
                
                
                auto res = read_header(_inputs[_file_count]);
                _files_positions[_file_count].reserve(res);
                _inputs[_file_count]->read(magic, sizeof(uint8_t)*4);
                
                std::stringstream strm;
                strm << "Opening surplus file number " << _file_count;
                putLog(strm.str());
            } else {
                running = false;
                break;
            }
        }
        
        if (checkId(magic, "VIDF")) {
            mlv_vidf_hdr_t video_frame_header;
            auto seek = _inputs[_file_count]->tellg();
            char * offset_ptr = (char *)(&video_frame_header) + 4;
            _inputs[_file_count]->read(offset_ptr, sizeof(mlv_vidf_hdr_t) - 4*sizeof(uint8_t));
            
            int64_t frameSize = video_frame_header.blockSize - video_frame_header.frameSpace - sizeof(mlv_vidf_hdr_t);
            
            _files_positions[_file_count].push_back(std::make_pair(video_frame_header.timestamp, seek));
            
            _inputs[_file_count]->seekg(video_frame_header.frameSpace + frameSize, std::ios::cur);
            //_inputs[_file_count]->seekg(frameSize, std::ios::cur);
            
        } else if (checkId(magic, "AUDF")) {
            mlv_audf_hdr_t audio_frame_header;
            auto seek = _inputs[_file_count]->tellg();
            char * offset_ptr = (char *)(&audio_frame_header) + 4;
            _inputs[_file_count]->read(offset_ptr, sizeof(mlv_audf_hdr_t) - 4*sizeof(uint8_t));
            
            int64_t frameSize = audio_frame_header.blockSize - audio_frame_header.frameSpace - sizeof(mlv_vidf_hdr_t);
            
            //_positions.push_back(std::make_pair(audio_frame_header.timestamp, seek));
            
            _inputs[_file_count]->seekg(audio_frame_header.frameSpace + frameSize, std::ios::cur);
            
            //_inputs[_file_count]->seekg(frameSize, std::ios::cur);
        } else if (checkId(magic, "RAWI")) {
            //putLog("RAWI");

			_inputs[_file_count]->seekg(sizeof(mlv_rawi_hdr_t) - 4 * sizeof(uint8_t), std::ios::cur);
        } else if (checkId(magic, "NULL")) {
            uint32_t blockSize;
            _inputs[_file_count]->read((char*)&blockSize, sizeof(uint32_t));
            _inputs[_file_count]->seekg(blockSize - 4*sizeof(uint8_t) - sizeof(uint32_t), std::ios::cur);
        } else {
            
            uint32_t blockSize;
            _inputs[_file_count]->read((char*)&blockSize, sizeof(uint32_t));
            _inputs[_file_count]->seekg(blockSize - 4*sizeof(uint8_t) - sizeof(uint32_t), std::ios::cur);
            
            //std::stringstream strm;
            //strm << "Unhandled block: " << magic << ", skipping " << blockSize << " bytes";
            //putLog(strm.str());
        }
        
    }
    
    running = true;
    size_t big_num = 0;
    for (int i = 0; i < _files_positions.size(); ++i) {
        //std::sort(positions.begin(), positions.end());
        auto& positions = _files_positions[i];
        big_num += positions.size();
        _reg_files_positions.reserve(big_num);
        
        for (auto& p : positions) {
            _reg_files_positions.push_back({{p.first, i}, p.second});
        }
    }
    std::sort(_reg_files_positions.begin(), _reg_files_positions.end());
    
    uint64_t timestamp_previous = 0;
    //for (int p = 0; p < _files_positions.size(); ++p) {
        size_t num = _reg_files_positions.size();
    
	_frame_count = num;
    
    //input->clear();
        
    //auto& _positions = _files_positions[p];
    _currentFrame = 0;
    for (size_t i = _frame_skip; i < num; ++i) {
        
		// sweet sweet scrub
		{
			std::lock_guard<std::mutex> lock(_scrub_mutex);
			if (_scrub_set) {
				_scrub_set = false;
				if (_scrub_target < num && _scrub_target >= 0) {
					std::stringstream strm;
					strm << "Seeked to frame " << _scrub_target << ".";
					putLog(strm.str());
					i = _scrub_target;
				}
			}
		}
        
        _currentFrame = i;
        
        auto& pos = _reg_files_positions[i];
        auto& input = _inputs[pos.first.second];
            
        input->clear();
        input->seekg(pos.second);
            
        mlv_vidf_hdr_t video_frame_header;
        char * offset_ptr = (char *)(&video_frame_header) + 4;
        input->read(offset_ptr, sizeof(mlv_vidf_hdr_t) - 4*sizeof(uint8_t));
            
        input->ignore(video_frame_header.frameSpace);
            
        uint64_t frameSize = video_frame_header.blockSize - video_frame_header.frameSpace - sizeof(mlv_vidf_hdr_t);
        
		auto ptr = inLock();
        if (ptr == nullptr) {
            break;
        }
            
        uint64_t bytesize = 0;
            
		input->read((char*)_upload_buffer, sizeof(uint16_t) * 7 * _width * _height / 8);
		bytesize += sizeof(uint16_t) * 7 * _width * _height / 8;
		cl::Event event;
		queue->enqueueWriteBuffer(*_input_buffer, CL_TRUE, 0, sizeof(uint16_t) * 7 * _width * _height / 8, _upload_buffer, NULL, &event);
		event.wait();

		_mlv_raw_map->setArg(0, *_input_buffer);
		_mlv_raw_map->setArg(1, ptr.get_buffer());
		queue->enqueueNDRangeKernel(*_mlv_raw_map, cl::NullRange, cl::NDRange(_width*_height/8, 1), cl::NullRange, NULL, &event);
		event.wait();
            
        inUnlock();
            
        if (frameSize - bytesize != 0) {
            putLog("Warning: less bytes read than blockSize.");
        }
            
        if (input->eof() || input->fail() || input->bad()) {
            putLog("Warning: stream bad.");
        }
            
        if (video_frame_header.timestamp < timestamp_previous) {
            putLog("Warning: timestamps non-monotonic.");
        }
        timestamp_previous = video_frame_header.timestamp;
    }

    release();
}

void mlvRawInput::moveToFrame(int frame) {
	std::lock_guard<std::mutex> lock(_scrub_mutex);
	_scrub_set = true;
	_scrub_target = frame;
}

int mlvRawInput::getFrameCount() {
	return _frame_count;
}

int mlvRawInput::getCurrentFrame() {
    return _currentFrame;
}

size_t mlvRawInput::read_header(std::shared_ptr<std::ifstream> input) {
    
    char ptr[5] = {"0000"};
    
    input->read(ptr, sizeof(uint8_t)*4);
    size_t cnt = input->gcount();
    
    bool foundMLVI = checkId((const char*)ptr, "MLVI");
    
    if (!foundMLVI) {
        throw std::runtime_error("Couldn't find MLVI block.");
    }
    
    mlv_file_hdr_t file_header;
    
    char * offset_ptr = (char *)(&file_header) + 4;
    
    input->read(offset_ptr, sizeof(mlv_file_hdr_t) - 4*sizeof(uint8_t));
    
    size_t cnt2 = input->gcount();
    
    if (input->eof() || input->fail() || input->bad() || cnt2 != sizeof(mlv_file_hdr_t) - 4*sizeof(uint8_t)) {
        throw std::runtime_error("Bad MLVI block.");
    }
    /*
    input->read(ptr, sizeof(uint8_t)*4);
    
    if (!checkId(ptr, "RAWI")) {
        throw std::runtime_error("Couldn't find RAWI block.");
    }
    
    mlv_rawi_hdr_t raw_header;
    offset_ptr = (char *)(&raw_header) + 4;
    input->read(offset_ptr, sizeof(mlv_rawi_hdr_t) - 4*sizeof(uint8_t));
    */
    return cnt;
}

bool mlvRawInput::event(std::shared_ptr<azure::Event> event) {
	static bool shift_pressed = false;

	if (event->isType("scrubClick")) {
		float x = event->getAttribute<float>("x");
		float y = event->getAttribute<float>("y");
		float w = event->getAttribute<float>("width");
		float diff = x / w;


		moveToFrame(diff * _frame_count);
	} else if (event->isType("keyDown")) {
		auto key = event->getAttribute<azure::Key>("key");
		std::stringstream strm;

		/*
         if (key == azure::Key::LShift || key == azure::Key::RShift) {
			shift_pressed = true;
		}
		if (shift_pressed) {
			if (key == azure::Key::Plus || key == azure::Key::KP_Plus) {
				_white += 100;
				strm << "White Point increased to " << _white << ".";
				putLog(strm.str());
			} else if (key == azure::Key::Minus || key == azure::Key::KP_Minus) {
				_white -= 100;
				strm << "White Point decreased to " << _white << ".";
			}
		} else {*/
        if (key == azure::Key::Plus || key == azure::Key::KP_Plus || key == azure::Key::Equals) {
				_black += 1;
            strm << "Black Point increased to " << _black << ".";
            putLog(strm.str());
				putLog(strm.str());
			} else if (key == azure::Key::Minus || key == azure::Key::KP_Minus) {
				_black -= 1;
                strm << "Black Point decreased to " << _black << ".";
                putLog(strm.str());
			}
		//}
		
	} else if (event->isType("keyUp")) {
		/*
         auto key = event->getAttribute<azure::Key>("key");

		if (key == azure::Key::LShift || key == azure::Key::RShift) {
			shift_pressed = false;
		}
         */
	}
	return false;
}
