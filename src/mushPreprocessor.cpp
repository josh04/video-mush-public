//
//  hdrPreprocess.cpp
//  video-mush
//
//  Created by Josh McNamee on 28/08/2015.
//
//

#include "mushPreprocessor.hpp"
#include <azure/events.hpp>
#include <azure/chrono.hpp>
#include <Mush Core/opencl.hpp>
#include "CubeLUT.h"

#include <Mush Core/fixedExposureProcess.hpp>
#include <Mush Core/scrubbableFrameGrabber.hpp>
#include "mlvRawInput.hpp"

namespace mush {

    void mushPreprocessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		assert(buffers.size() == 1);
		auto input = std::dynamic_pointer_cast<mush::frameGrabber>(buffers.begin()[0]);
		input->getDetails(config);
		if (input == nullptr) {
			throw std::runtime_error("Non-framegrabber passed to preprocessor.");
		}
        lock_fps = config.lock_fps;
        frame_time = 1.0f/config.fps;
        
        input->getParams(_input_width, _input_height, _size);
        queue = context->getQueue();
        _input = input;
        _scrub = std::dynamic_pointer_cast<scrubbableFrameGrabber>(_input.get());
        setTagInGuiName(input->getTagInGuiName());
        
        _inputEngine = input->inputEngine();
        
        if (config.resize) {
            _width = config.resize_width;
            _height = config.resize_height;
            resize_kernel = context->getKernel("resize");
            resize_frame = context->floatImage(_width, _height);
        } else {
            _width = _input_width;
            _height = _input_height;
        }

		_darken = config.darken;
        
        rgba = context->getKernel("rgba");
        planarRGBtoRGBA = context->getKernel("planarRGBtoRGBA");
        planarYUVtoRGBA = context->getKernel("planarYUVtoRGBA");
        convert = context->getKernel("convert");
        convert10 = context->getKernel("convert10");
        bgraToRgba = context->getKernel("bgraToRgba");
        decodeUDP = context->getKernel("decodeUDP");
        decodeMLV = context->getKernel("short_buffer_to_image");
        //debayer = context->getKernel("simple_debayer");
		exposure_kernel = context->getKernel("exposure");
        _func = config.func;
        gammacorrect = config.gammacorrect;
        switch (config.func) {
            case mush::transfer::g8:
                transfer_kernel = context->getKernel("decodePTF4");
                transfer_kernel->setArg(2, 10000.0f);
                break;
            case mush::transfer::pq:
                transfer_kernel = context->getKernel("pqdecode");
                //transfer_kernel->setArg(2, 1.0);
                break;
            case mush::transfer::srgb:
                transfer_kernel = context->getKernel("decodeSRGB");
                break;
            case mush::transfer::logc:
            {
                transfer_kernel = context->getKernel("logC");
                _cube = std::make_shared<CubeLUT>();
                _cube_image = _cube->get(context, config.resourceDir);
                transfer_kernel->setArg(2, *_cube_image);
            }
                break;
            case mush::transfer::gamma:
                transfer_kernel = context->getKernel("gamma");
                transfer_kernel->setArg(2, config.gammacorrect);
                break;
            case transfer::rec709:
                transfer_kernel = context->getKernel("decodeRec709");
                break;
            default:
            case mush::transfer::linear:
                transfer_kernel = context->getKernel("copyImage");
                break;
        }
        
        _filetype = config.filetype;
        /*
        if (_inputEngine == mush::inputEngine::mlvRawInput) {
            rgb16Buffer = context->buffer(sizeof(uint16_t) * _input_width * _input_height);
        }
        */
        if (_inputEngine == mush::inputEngine::rgb16bitInput) {
            rgb16Buffer = context->buffer(sizeof(uint16_t) * 3 * _input_width*_input_height, CL_MEM_WRITE_ONLY);
            planarRGBtoRGBA->setArg(0, *rgb16Buffer);
            planarRGBtoRGBA->setArg(2, _input_width);
            planarRGBtoRGBA->setArg(3, _input_height);
        }
        
        if (_inputEngine == mush::inputEngine::yuv10bitInput) {
            rgb16Buffer = context->buffer(sizeof(uint16_t) * 2 * _input_width*_input_height, CL_MEM_WRITE_ONLY);
            planarYUVtoRGBA->setArg(0, *rgb16Buffer);
            planarYUVtoRGBA->setArg(2, _input_width);
            planarYUVtoRGBA->setArg(3, _input_height);
        }
        
        if (_filetype == mush::filetype::pfmFiletype) {
            rgbaInput = context->floatImage(_input_width, _input_height);
            rgba->setArg(0, *rgbaInput);
        }
        
        if (_filetype == mush::filetype::mergeTiffFiletype) {
            if (_size == sizeof(char)) {
                rgbaInput = context->intImage(_input_width, _input_height);
            } else if (_size == sizeof(short)) {
                rgbaInput = context->int16bitImage(_input_width, _input_height);
            } else { // if (_size == sizeof(float)) {
                rgbaInput = context->floatImage(_input_width, _input_height);
            }
            rgba->setArg(0, *rgbaInput);
        }
        
        if (_inputEngine == mush::inputEngine::jpegInput) {
            rgbaInput = context->intImage(_input_width, _input_height);
            rgba->setArg(0, *rgbaInput);
        }
        
        if (_inputEngine == mush::inputEngine::flare10Input) {
            if (_input_width % 48 > 0) {
                _length = _input_width + (48 - (_input_width % 48));
            } else {
                _length = _input_width;
            }
            rgb16Buffer = context->buffer(128*(_length/48)*_input_height);
        }
        
        if (_inputEngine == mush::inputEngine::flareInput || _inputEngine == mush::inputEngine::canonInput) {
            rgbaInput = context->intNotNormalisedImage(_input_width/2, _input_height);
            convert->setArg(1, *rgbaInput);
            convert->setArg(2, 0);
        }
        
        if (_inputEngine == mush::inputEngine::udpInput) {
            udpBuffer = context->buffer(_input_width * _input_height * 2 * sizeof(uint8_t));
            
            decodeUDP->setArg(1, *udpBuffer);
        }
        
        const int buffers_no = 2;
        for (int i = 0; i < buffers_no; i++) {
            if (_inputEngine == mush::inputEngine::videoInput) {
                output_frame = context->intImage(_input_width, _input_height);
            } else if (_filetype == mush::filetype::exrFiletype || _inputEngine == mush::inputEngine::fastEXRInput || _inputEngine == mush::inputEngine::testCardInput || _inputEngine == mush::inputEngine::singleEXRInput) {
                output_frame = context->halfImage(_input_width, _input_height);
            } else if (_inputEngine == mush::inputEngine::externalInput) {
                if (config.input_pix_fmt == mush::input_pix_fmt::char_4channel) {
                    output_frame = context->intBGRAImage(_input_width, _input_height);
                } else if (config.input_pix_fmt == mush::input_pix_fmt::half_4channel) {
                    output_frame = context->halfImage(_input_width, _input_height);
                } else {
                    output_frame = context->floatImage(_input_width, _input_height);
                }
            } else {
                output_frame = context->floatImage(_input_width, _input_height);
            }
        }
        
        addItem(context->floatImage(_width, _height));
        addItem(context->floatImage(_width, _height));
        
    }

	void mushPreprocessor::go() {

		SetThreadName("preprocess");

		while (_input->good()) {
			process();
		}

		release();
	}

    void mushPreprocessor::process() {

		cl::size_t<3> origin;
		cl::size_t<3> region;
		origin[0] = 0; origin[1] = 0; origin[2] = 0;
		region[0] = _input_width; region[1] = _input_height; region[2] = 1;

            cl::Event event;
            
            auto buf = _input->outLock();
            
            if (buf == nullptr) {
                return;
            }

			// mlv input hands off a cl buffer, crashing this otherwise
			unsigned char * ptr = (buf.get_type() == mush::buffer::type::host_pointer) ? (unsigned char *)buf.get_pointer() : nullptr;
            
            auto& in = inLock();
            if (in == nullptr) {
				return;
            }
            
            if (buf.has_camera_position()) {
                in.set_camera_position(buf.get_camera_position(), buf.get_theta_phi_fov());
            } else {
                in.set_no_camera_position();
            }
            
            if (_inputEngine == mush::inputEngine::mlvRawInput) {
				/*
                queue->enqueueWriteBuffer(*rgb16Buffer, CL_TRUE, 0, _input_width*_input_height*sizeof(uint16_t), ptr, NULL, &event);
                event.wait();
                _input->outUnlock();
                
                */
                //decodeMLV->setArg(0, *rgb16Buffer);
				auto buffer = buf.get_buffer();

				decodeMLV->setArg(0, buffer);
                decodeMLV->setArg(1, *output_frame);
                decodeMLV->setArg(2, _input_width);
                decodeMLV->setArg(3, _input_height);
                
                auto mlv = std::dynamic_pointer_cast<mush::mlvRawInput>(_input.get());
                if (mlv != nullptr) {
                    decodeMLV->setArg(4, mlv->getBlackPoint());
					decodeMLV->setArg(5, mlv->getWhitePoint());
                }
                
                queue->enqueueNDRangeKernel(*decodeMLV, cl::NullRange, cl::NDRange(_input_width, _input_height), cl::NullRange, NULL, &event);
                event.wait();

				_input->outUnlock();
            } else if (_inputEngine == mush::inputEngine::rgb16bitInput) {
                queue->enqueueWriteBuffer(*rgb16Buffer, CL_TRUE, 0, _input_width*_input_height*sizeof(uint16_t) * 3, ptr, NULL, &event);
                event.wait();
                _input->outUnlock();
                
                planarRGBtoRGBA->setArg(1, *output_frame);
                queue->enqueueNDRangeKernel(*planarRGBtoRGBA, cl::NullRange, cl::NDRange(_input_width, _input_height), cl::NullRange, NULL, &event);
                event.wait();
            } else if (_inputEngine == mush::inputEngine::yuv10bitInput) {
                queue->enqueueWriteBuffer(*rgb16Buffer, CL_TRUE, 0, _input_width*_input_height*sizeof(uint16_t) * 2, ptr, NULL, &event);
                event.wait();
                _input->outUnlock();
                
                planarYUVtoRGBA->setArg(1, *output_frame);
                queue->enqueueNDRangeKernel(*planarYUVtoRGBA, cl::NullRange, cl::NDRange(_input_width/2, _input_height), cl::NullRange, NULL, &event);
                event.wait();
            } else if (_filetype == mush::filetype::pfmFiletype || _inputEngine == mush::inputEngine::jpegInput || _filetype == mush::filetype::mergeTiffFiletype) {
                queue->enqueueWriteImage(*rgbaInput, CL_TRUE, origin, region, 0, 0, ptr, NULL, &event);
                event.wait();
                _input->outUnlock();
                
                rgba->setArg(1, *output_frame);
                queue->enqueueNDRangeKernel(*rgba, cl::NullRange, cl::NDRange(_input_width, _input_height), cl::NullRange, NULL, &event);
                event.wait();
            } else if (_inputEngine == mush::inputEngine::canonInput) {
                region[0] = _input_width / 2; region[1] = _input_height; region[2] = 1;
                queue->enqueueWriteImage(*rgbaInput, CL_TRUE, origin, region, 0, 0, ptr, NULL, &event);
                convert->setArg(0, *output_frame);
                queue->enqueueNDRangeKernel(*convert, cl::NullRange, cl::NDRange(_input_width / 2, _input_height), cl::NullRange, NULL, &event);
                event.wait();
                _input->outUnlock();
            } else if (_inputEngine == mush::inputEngine::flare10Input) {
                //void * ptr2 = malloc(128*(length/48)*_input_height);
                queue->enqueueWriteBuffer(*rgb16Buffer, CL_TRUE, 0, 128*(_length/48)*_input_height, ptr, NULL, &event);
                convert10->setArg(0, *output_frame);
                convert10->setArg(1, *rgb16Buffer);
                queue->enqueueNDRangeKernel(*convert10, cl::NullRange, cl::NDRange(_length/6, _input_height), cl::NullRange, NULL, &event);
                event.wait();
                _input->outUnlock();
            } else if (_inputEngine == mush::inputEngine::udpInput) {
                
                queue->enqueueWriteBuffer(*udpBuffer, CL_TRUE, 0, _input_width*_input_height * 2 * sizeof(uint8_t), ptr, NULL, &event);
                event.wait();
                decodeUDP->setArg(0, *output_frame);
                
                queue->enqueueNDRangeKernel(*decodeUDP, cl::NullRange, cl::NDRange(_input_width / 2, _input_height), cl::NullRange, NULL, &event);
                event.wait();
                
                _input->outUnlock();
            } else {
                queue->enqueueWriteImage(*output_frame, CL_TRUE, origin, region, 0, 0, ptr, NULL, &event);
                event.wait();
                _input->outUnlock();
            }
            inUnlock();
            
    }

    void mushPreprocessor::inUnlock() {
        if (resize_kernel != nullptr) {
            resize_kernel->setArg(0, *output_frame);
            if (transfer_kernel != nullptr) {
                resize_kernel->setArg(1, *resize_frame);
            } else {
                resize_kernel->setArg(1, _getImageMem(next));
            }
            cl::Event event;
            queue->enqueueNDRangeKernel(*resize_kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
        }
        
        if (transfer_kernel != nullptr) {
            if (resize_kernel != nullptr) {
                transfer_kernel->setArg(0, *resize_frame);
            } else {
                transfer_kernel->setArg(0, *output_frame);
            }
            transfer_kernel->setArg(1, _getImageMem(next));
            cl::Event event;
            queue->enqueueNDRangeKernel(*transfer_kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
			exposure_kernel->setArg(0, _getImageMem(next));
			exposure_kernel->setArg(1, _getImageMem(next));
			exposure_kernel->setArg(2, _darken);
			queue->enqueueNDRangeKernel(*exposure_kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();
        }
        
        mush::imageBuffer::inUnlock();
        /*
        if (_scrub != nullptr) {
            auto event2 = std::unique_ptr<azure::Event>(new azure::Event("setGuiText", 0));
            
            event2->setAttribute<int>("tag", getTagInGuiIndex());
            event2->setAttribute<std::string>("label", "framecount");
            
            int y = _scrub->getCurrentFrame();
            int t = _scrub->getFrameCount();
            std::stringstream strm;
            strm << y << " / " << t;
            
            event2->setAttribute<std::string>("text", strm.str().c_str());
            
            azure::Events::Push(std::move(event2));
        }
		*/

		if (lock_fps) {

			azure::Duration delta = azure::Clock::now() - _previous_clock;
			azure::Duration delay = std::max(azure::Duration(frame_time) - delta, azure::Duration(1e-3)); // Max FPS 1/1000. Give the OS a break.

			std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::nanoseconds>(delay));

			_previous_clock = azure::Clock::now();
		}
    }
    
    void mushPreprocessor::setTransferFuncParameters() {
        switch (_func) {
            case mush::transfer::g8:
                transfer_kernel->setArg(2, 10000.0f);
                break;
            case mush::transfer::pq:
                //transfer_kernel->setArg(2, 1.0);
                break;
            case mush::transfer::srgb:
                break;
            case mush::transfer::logc:
            {
                transfer_kernel->setArg(2, *_cube_image);
            }
                break;
            case mush::transfer::gamma:
                transfer_kernel->setArg(2, gammacorrect);
                break;
            case transfer::rec709:
                break;
            default:
            case mush::transfer::linear:
                break;
        }
    }
    
    
    bool mushPreprocessor::event(std::shared_ptr<azure::Event> event) {
        if (_scrub != nullptr) {
            if (event->isType("scrubClick")) {
                if ((int)event->getAttribute<float>("tag") == getTagInGuiIndex()) {
                    float x = event->getAttribute<float>("x");
                    float y = event->getAttribute<float>("y");
                    float w = event->getAttribute<float>("width");
                    float diff = x / w;
                    
                    
                    _scrub->moveToFrame(diff * _scrub->getFrameCount());
                }
            }
        }
        return false;
    }
}
