#pragma once
//
//  ffmpegEncodeDecode.hpp
//  parcore
//
//  Created by Josh McNamee on 24/06/2015.
//  Copyright (c) 2015 Josh McNamee. All rights reserved.
//


extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}


#ifndef __APPLE__
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/opencl.hpp>
#include "avcodecEncoder.hpp"
#include "ConfigStruct.hpp"
#include "yuvDepthClampProcess.hpp"
#else

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/opencl.hpp>
#include <Video Mush/avcodecEncoder.hpp>
#include <Video Mush/ConfigStruct.hpp>
#include "yuvDepthClampProcess.hpp"
#endif

#include "testImageGenerator.hpp"


namespace mush {
	class testSwscale : public mush::imageProcess, public ffmpegWrapper {
	public:

		testSwscale(avcodec_codec e, bool use_clamp) : mush::imageProcess(), ffmpegWrapper(), _e(e), use_clamp(use_clamp) {
		}

		~testSwscale() {
		}


		void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
			assert(buffers.size() == 1);

			input = castToImage(buffers.begin()[0]);

			input->getParams(_width, _height, _size);

			//_width = 4;
			//_height = 4;

			std::array<std::array<std::array<float, 4>, 4>, 4> temp_arr =
			{ {
				{{ {{ 0.0, 0.0, 0.0, 1.0 }}, {{ 0.0, 0.0, 0.0, 1.0 }}, {{ 0.5, 0.5, 0.5, 1.0 }}, {{ 0.5, 0.5, 0.5, 1.0 }} }},
				{{ {{ 0.0, 0.0, 0.0, 1.0 }}, {{ 0.0, 0.0, 0.0, 1.0 }}, {{ 0.5, 0.5, 0.5, 1.0 }}, {{ 0.5, 0.5, 0.5, 1.0 }} }},
				{{ {{ 0.5, 0.0, 0.5, 1.0 }}, {{ 0.0, 0.5, 0.5, 1.0 }}, {{ 1.0, 1.0, 1.0, 1.0 }}, {{ 1.0, 1.0, 1.0, 1.0 }} }},
				{{ {{ 0.5, 0.0, 0.5, 1.0 }}, {{ 0.0, 0.5, 0.5, 1.0 }}, {{ 1.0, 1.0, 1.0, 1.0 }}, {{ 1.0, 1.0, 1.0, 1.0 }} }},
			} };

			//float * temp = (float *)malloc(_width*_height * 4 * sizeof(float));
			
			_test = std::make_shared<testImageGenerator>(context, 4, 4);
			_test->setData((float *)temp_arr.data());

			queue = context->getQueue();

			addItem(context->floatImage(_width, _height));
            auto pan = sws_getCoefficients(1);
            auto pan2 = sws_getCoefficients(5);
            auto pan3 = sws_getCoefficients(777);
			switch (_e) {
			case avcodec_codec::x264:
				fmt_in = AV_PIX_FMT_YUV422P;
				fmt_mid = AV_PIX_FMT_YUV420P;
                    _clamp = std::make_shared<mush::yuvDepthClampProcess>(8, 1.0f, transfer::linear);
				break;
			case avcodec_codec::x265:
				fmt_in = AV_PIX_FMT_YUV422P10LE;
				fmt_mid = AV_PIX_FMT_YUV420P10LE;
				_clamp = std::make_shared<mush::yuvDepthClampProcess>(10, 1.0f, transfer::linear);
				break;
			case avcodec_codec::vpx:
				fmt_in = AV_PIX_FMT_YUV422P12LE;
				fmt_mid = AV_PIX_FMT_YUV420P12LE;
				_clamp = std::make_shared<mush::yuvDepthClampProcess>(12, 1.0f, transfer::linear);
				break;
			case avcodec_codec::prores:
				fmt_in = AV_PIX_FMT_YUV422P10LE;
				fmt_mid = AV_PIX_FMT_YUV422P10LE;
				_clamp = std::make_shared<mush::yuvDepthClampProcess>(10, 1.0f, transfer::linear);
				break;
			}

			_clamp->init(context, input);
			//_clamp->init(context, _test);

            
            fmt_pre_in = AV_PIX_FMT_RGBA64;

			fmt_out = AV_PIX_FMT_RGBA64;
            
            sws_ctx_pre_in = std::shared_ptr<SwsContext>(sws_getContext(_width, _height, fmt_pre_in, _width, _height, fmt_in, SWS_BILINEAR | SWS_ACCURATE_RND, NULL, NULL, NULL), [](SwsContext * s) {sws_freeContext(s); });

			sws_ctx_in = std::shared_ptr<SwsContext>(sws_getContext(_width, _height, fmt_in, _width, _height, fmt_mid, SWS_BILINEAR | SWS_ACCURATE_RND, NULL, NULL, NULL), [](SwsContext * s) {sws_freeContext(s); });

			sws_ctx_out = std::shared_ptr<SwsContext>(sws_getContext(_width, _height, fmt_mid, _width, _height, fmt_out, SWS_BILINEAR | SWS_ACCURATE_RND, NULL, NULL, NULL), [](SwsContext * s) {sws_freeContext(s); });

            int * in_table, * out_table;
            int in_scale, out_scale, brightness, contrast, saturation;
            sws_getColorspaceDetails(sws_ctx_pre_in.get(), &in_table, &in_scale, &out_table, &out_scale, &brightness, &contrast, &saturation);
            
            int * in_table2, * out_table2;
            int in_scale2, out_scale2, brightness2, contrast2, saturation2;
            sws_getColorspaceDetails(sws_ctx_in.get(), &in_table2, &in_scale2, &out_table2, &out_scale2, &brightness2, &contrast2, &saturation2);
            
            sws_setColorspaceDetails(sws_ctx_pre_in.get(), sws_getCoefficients(SWS_CS_ITU709), 0, sws_getCoefficients(SWS_CS_ITU709), 0, 0, 1 << 16, 1 << 16);
            
            sws_setColorspaceDetails(sws_ctx_in.get(), sws_getCoefficients(SWS_CS_ITU709), 0, sws_getCoefficients(SWS_CS_ITU709), 0, 0, 1 << 16, 1 << 16);
            
            sws_setColorspaceDetails(sws_ctx_out.get(), sws_getCoefficients(SWS_CS_ITU709), 0, sws_getCoefficients(SWS_CS_ITU709), 0, 0, 1 << 16, 1 << 16);

			size_t int_width = _clamp->get_buffer_int_width();


			int clamp_image_size = _clamp->getMapSize() * 2;
			in_ptr[0] = context->hostReadBuffer(clamp_image_size);
			in_ptr[1] = in_ptr[0] + _width*_height*int_width;
			in_ptr[2] = in_ptr[1] + (_width*_height / 2)*int_width;

			in_linesize[0] = _width*int_width;
			in_linesize[1] = (_width / 2)*int_width;
			in_linesize[2] = (_width / 2)*int_width;


			mid_ptr[0] = (uint8_t *)malloc(_width*_height*int_width*1.5);

			mid_ptr[1] = mid_ptr[0] + _width*_height*int_width;

			if (fmt_mid == AV_PIX_FMT_YUV420P12LE || fmt_mid == AV_PIX_FMT_YUV420P10LE || fmt_mid == AV_PIX_FMT_YUV420P) {
				mid_ptr[2] = mid_ptr[1] + (_width*_height / 4)*int_width;
			} else if (fmt_mid == AV_PIX_FMT_YUV422P10LE) {
				mid_ptr[2] = mid_ptr[1] + (_width*_height / 2)*int_width;
			} else {
				throw std::runtime_error("Unusual pixel format.");
			}

			mid_linesize[0] = _width*int_width;
			mid_linesize[1] = (_width / 2)*int_width;
			mid_linesize[2] = (_width / 2)*int_width;

			out_ptr = context->hostWriteBuffer(_width*_height * sizeof(uint16_t) * 4);
			out_linesize = _width * sizeof(uint16_t) * 4;
            
            pre_in_ptr = context->hostReadBuffer(_width*_height * sizeof(uint16_t) * 4);
            pre_in_linesize = _width * sizeof(uint16_t) * 4;

			upload = context->int16bitImage(_width, _height);
			upload2 = context->int16bitImage(_width, _height);

            transfer_kernel = context->getKernel("copyImage");
            fix_rgb = context->getKernel("fix_rgb");
		}

		void process() {
            
            cl::Event event;
            cl::size_t<3> origin, region;
            origin[0] = 0; origin[1] = 0; origin[2] = 0;
            region[0] = _width; region[1] = _height; region[2] = 1;
            
            if (use_clamp) {
				//auto in_dis = input->outLock();
                _clamp->process();
				//input->outUnlock();

                auto clampBuffer = _clamp->outLock();
                if (clampBuffer == nullptr) {
                    release();
                    return;
                }

                int clamp_image_size = _clamp->getMapSize() * 2;

                queue->enqueueReadBuffer(clampBuffer.get_buffer(), CL_TRUE, 0, clamp_image_size, in_ptr[0], NULL, &event);
                event.wait();
                
                /*
                uint8_t ym_x = 0, ym_n = 255;
                uint8_t um_x = 0, um_n = 255;
                uint8_t vm_x = 0, vm_n = 255;
                for (int j = 0; j < _height; ++j) {
                    
                    for (int i = 0; i < _width; ++i) {
                        ym_x = std::max(ym_x, in_ptr[0][i + j * _width]);
                        ym_n = std::min(ym_n, in_ptr[0][i + j * _width]);
                        
                        um_x = std::max(um_x, in_ptr[1][(i + j * _width) / 2]);
                        um_n = std::min(um_n, in_ptr[1][(i + j * _width) / 2]);
                        
                        vm_x = std::max(vm_x, in_ptr[2][(i + j * _width) / 2]);
                        vm_n = std::min(vm_n, in_ptr[2][(i + j * _width) / 2]);
                    }
                    
                }
                
                std::cout << "ymax: " << (int)ym_x << " ymin: " << (int)ym_n << std::endl;
                std::cout << "umax: " << (int)um_x << " umin: " << (int)um_n << std::endl;
                std::cout << "vmax: " << (int)vm_x << " vmin: " << (int)vm_n << std::endl;
                std::cout << std::endl;
                */

                _clamp->outUnlock();

            } else {
                auto in = input->outLock();
				//auto in = _test->outLock();
				if (in == nullptr) {
					release();
					return;
				}
                
                transfer_kernel->setArg(0, in.get_image());
                transfer_kernel->setArg(1, *upload);
                
                queue->enqueueNDRangeKernel(*transfer_kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
                event.wait();
                
                queue->enqueueReadImage(*upload, CL_TRUE, origin, region, 0, 0, pre_in_ptr, NULL, &event);
                event.wait();
				/*
				std::stringstream strm;
				auto pre_in_ptr2 = (uint16_t *)pre_in_ptr;
				strm << "rgb" << std::endl;
				for (int i = 0; i < _height; ++i) {
					for (int j = 0; j < _width; ++j) {
						strm << (int)pre_in_ptr2[(i * _width + j) * 4] << " " << (int)pre_in_ptr2[(i * _width + j) * 4 + 1] << " " << (int)pre_in_ptr2[(i * _width + j) * 4 + 2] << " | ";
					}
					strm << std::endl;
				}

				putLog(strm.str());
				*/
				sws_setColorspaceDetails(sws_ctx_pre_in.get(), sws_getCoefficients(SWS_CS_ITU709), 0, sws_getCoefficients(SWS_CS_ITU709), 0, 0, 1 << 16, 1 << 16);

                sws_scale(sws_ctx_pre_in.get(), &pre_in_ptr, &pre_in_linesize, 0, _height, in_ptr, in_linesize);
                /*
                uint8_t ym_x = 0, ym_n = 255;
                uint8_t um_x = 0, um_n = 255;
                uint8_t vm_x = 0, vm_n = 255;
                for (int j = 0; j < _height; ++j) {
                    
                    for (int i = 0; i < _width; ++i) {
                        ym_x = std::max(ym_x, in_ptr[0][i + j * _width]);
                        ym_n = std::min(ym_n, in_ptr[0][i + j * _width]);
                        
                        um_x = std::max(um_x, in_ptr[1][(i + j * _width) / 2]);
                        um_n = std::min(um_n, in_ptr[1][(i + j * _width) / 2]);
                        
                        vm_x = std::max(vm_x, in_ptr[2][(i + j * _width) / 2]);
                        vm_n = std::min(vm_n, in_ptr[2][(i + j * _width) / 2]);
                    }
                    
                }
                
                std::cout << "ymax: " << (int)ym_x << " ymin: " << (int)ym_n << std::endl;
                std::cout << "umax: " << (int)um_x << " umin: " << (int)um_n << std::endl;
                std::cout << "vmax: " << (int)vm_x << " vmin: " << (int)vm_n << std::endl;
                std::cout << std::endl;
                */
                input->outUnlock();
				//_test->outUnlock();
            }
			/*
			std::stringstream strm;

			if (use_clamp) {
				strm << "clamp" << std::endl;
			} else {
				strm << "swscale" << std::endl;
			}

			strm << "y" << std::endl;
			for (int i = 0; i < _height; ++i) {
				for (int j = 0; j < _width; ++j) {
					strm << (int)in_ptr[0][(i * _width + j)] << " | ";
				}
				strm << std::endl;
			}


			strm << "u" << std::endl;
			for (int i = 0; i < _height; ++i) {
				for (int j = 0; j < _width / 2; ++j) {
					strm << (int)in_ptr[0][(_width * _height) + (i * _width/2 + j)] << " | ";
				}
				strm << std::endl;
			}

			strm << "v" << std::endl;
			for (int i = 0; i < _height; ++i) {
				for (int j = 0; j < _width / 2; ++j) {
					strm << (int)in_ptr[0][(int)(_width * _height * 1.5) + (i * _width/2 + j)] << " | ";
				}
				strm << std::endl;
			}

			*/
            sws_setColorspaceDetails(sws_ctx_in.get(), sws_getCoefficients(SWS_CS_ITU709), 0, sws_getCoefficients(SWS_CS_ITU709), 0, 0, 1 << 16, 1 << 16);
            
			sws_scale(sws_ctx_in.get(), in_ptr, in_linesize, 0, _height, mid_ptr, mid_linesize);
			/*

			strm << "y2" << std::endl;
			for (int i = 0; i < _height; ++i) {
				for (int j = 0; j < _width; ++j) {
					strm << (int)mid_ptr[0][(i * _width + j)] << " | ";
				}
				strm << std::endl;
			}


			strm << "u2" << std::endl;
			for (int i = 0; i < _height / 2; ++i) {
				for (int j = 0; j < _width / 2; ++j) {
					strm << (int)mid_ptr[0][(_width * _height) + (i * _width / 2 + j)] << " | ";
				}
				strm << std::endl;
			}

			strm << "v2" << std::endl;
			for (int i = 0; i < _height /2; ++i) {
				for (int j = 0; j < _width / 2; ++j) {
					strm << (int)mid_ptr[0][(int)(_width * _height * 1.25) + (i * _width / 2 + j)] << " | ";
				}
				strm << std::endl;
			}
			*/

			sws_setColorspaceDetails(sws_ctx_out.get(), sws_getCoefficients(SWS_CS_ITU709), 0, sws_getCoefficients(SWS_CS_ITU709), 0, 0, 1 << 16, 1 << 16);

			sws_scale(sws_ctx_out.get(), mid_ptr, mid_linesize, 0, _height, &out_ptr, &out_linesize);
			/*
			auto out_ptr2 = (uint16_t *)out_ptr;
			strm << "rgb" << std::endl;
			for (int i = 0; i < _height; ++i) {
				for (int j = 0; j < _width; ++j) {
					strm << (int)out_ptr2[(i * _width + j)*4] << " | " << (int)out_ptr2[(i * _width + j )*4 + 1] << " | " << (int)out_ptr2[(i * _width + j)*4 + 2] << " | ";
				}
				strm << std::endl;
			}

			putLog(strm.str());
			*/

            //sws_scale(sws_ctx_out.get(), in_ptr, in_linesize, 0, _height, &out_ptr, &out_linesize);

			queue->enqueueWriteImage(*upload2, CL_TRUE, origin, region, 0, 0, out_ptr, NULL, &event);
			event.wait();
            
            inLock();
			transfer_kernel->setArg(0, *upload2);
			transfer_kernel->setArg(1, _getImageMem(0));
			
			queue->enqueueNDRangeKernel(*transfer_kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();
            
            if (use_clamp) {
                fix_rgb->setArg(0, _getImageMem(0));
                fix_rgb->setArg(1, _getImageMem(0));
                queue->enqueueNDRangeKernel(*fix_rgb, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
                event.wait();
                
            }
            
			inUnlock();

		}

	private:
		cl::CommandQueue * queue = nullptr;

        std::shared_ptr<mush::imageBuffer> input = nullptr;
        mush::registerContainer<mush::yuvDepthClampProcess> _clamp;
        
        std::shared_ptr<SwsContext> sws_ctx_pre_in = nullptr;
		std::shared_ptr<SwsContext> sws_ctx_in = nullptr;
		std::shared_ptr<SwsContext> sws_ctx_out = nullptr;
        
        
        std::shared_ptr<SwsContext> sws_ctx_mid_1 = nullptr;
        
        std::shared_ptr<SwsContext> sws_ctx_mid_2 = nullptr;

        uint8_t * pre_in_ptr;
        int pre_in_linesize;
        
		uint8_t * in_ptr[3];
		int in_linesize[3];

		uint8_t * mid_ptr[3];
		int mid_linesize[3];

		uint8_t * out_ptr;
		int out_linesize;

		cl::Image2D * upload = nullptr;
		cl::Image2D * upload2 = nullptr;
        cl::Kernel * transfer_kernel = nullptr;
        cl::Kernel * fix_rgb = nullptr;
        
		avcodec_codec _e;

		AVPixelFormat fmt_pre_in, fmt_in, fmt_mid, fmt_out;
        
        bool use_clamp = true;

		mush::registerContainer<mush::testImageGenerator> _test;
	};
}
