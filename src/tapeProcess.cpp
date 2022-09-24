//
//  tapeProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 16/04/2016.
//
//

#include <stdio.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <random>
#include <algorithm>


#include <Mush Core/opencl.hpp>
#include "tapeProcess.hpp"

#include <sstream>

namespace mush {
    
    void tapeProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        
        _tape_interlace_on = context->getKernel("tape_interlace_on");
        _tape_interlace_off = context->getKernel("tape_interlace_off");
        
        _tape_convert = context->getKernel("tape_convert");
        _tape_read = context->getKernel("tape_read");
        
        _buffer = castToImage(buffers.begin()[0]);
        
        _buffer->getParams(_width, _height, _size);
        
        _temp_image = context->floatImage(_width, _height);
        _tape_image = context->floatImage(_width, _height * 7);
        
        _line_offsets = context->buffer(_height * sizeof(cl_float));
        _line_offsets_host = (float *)context->hostWriteBuffer(_height * sizeof(cl_float));
        
        _line_offset_linger = std::vector<int>(_height);
        _line_offset_linger_next = std::vector<int>(_height);
        _line_offset_rand = std::vector<int>(_height);
        
        addItem(context->floatImage(_width, _height));
        
        queue = context->getQueue();
    }
    
    void tapeProcess::process() {
        inLock();
        auto input = _buffer->outLock();
        if (input == nullptr) {
            release();
            return;
        }
        
        cl::Event event;
        
        
        _tape_interlace_on->setArg(0, input.get_image());
        _tape_interlace_on->setArg(1, *_temp_image);
        
        queue->enqueueNDRangeKernel(*_tape_interlace_on, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        
        _tape_convert->setArg(0, *_temp_image);
        _tape_convert->setArg(1, *_tape_image);
        
        queue->enqueueNDRangeKernel(*_tape_convert, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        static int i = 0;
        
        auto bool_rand = std::uniform_int_distribution<int>(-10,10);
        
        i = i + bool_rand(gen);
        
        tape_shift_offset += tape_shift_master * _height;
        tape_shift_offset = tape_shift_offset * 0.8f;
        
        auto bool_rand2 = std::uniform_real_distribution<float>(0,360);
        auto bool_rand_norm = std::normal_distribution<float>(-180,180);
        tape_shift_master -= sin(bool_rand_norm(gen) * M_PI/180.0f) * 0.01f;
        
        cl_float4 tape_shift_previous = {tape_shift_red * _height, tape_shift_green * _height, tape_shift_blue * _height, 0.0f};
        
        tape_shift_red = 0.0f; //bool_rand(gen) / 100.0f;
        tape_shift_green = (1.5 * tape_shift_green + 0.5 * sin(bool_rand2(gen) * M_PI/180.0f) * 0.05f) / 2.0;
        tape_shift_blue = (1.5 * tape_shift_blue + 0.5 * sin(bool_rand2(gen) * M_PI/180.0f) * 0.05f) / 2.0f;
        /*
        auto uint_rand = std::uniform_int_distribution<cl_uint>(0,UINT_MAX);
        cl_uint2 seed;
        seed.x = uint_rand(gen);
        seed.y = uint_rand(gen);
         */
        
        adjust_alignment();
        
        cl_float4 tape_align = {tape_align_red, tape_align_green, tape_align_blue, 0.0f};
        cl_float4 tape_shift = {tape_shift_red, tape_shift_green, tape_shift_blue, 0.0f};
        
        _tape_read->setArg(0, *_tape_image);
        _tape_read->setArg(1, *_temp_image);
        _tape_read->setArg(2, tape_align_master);
        _tape_read->setArg(3, tape_shift_master);
        _tape_read->setArg(4, tape_align);
        _tape_read->setArg(5, tape_shift);
        _tape_read->setArg(6, tape_shift_previous);
        _tape_read->setArg(7, tape_shift_offset);
        generate_offsets();
        _tape_read->setArg(8, *_line_offsets);
        
        queue->enqueueNDRangeKernel(*_tape_read, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        
        _tape_interlace_off->setArg(0, *_temp_image);
        _tape_interlace_off->setArg(1, _getImageMem(0));
        
        queue->enqueueNDRangeKernel(*_tape_interlace_off, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        
        _buffer->outUnlock();
        inUnlock();
    }
    
    void tapeProcess::generate_offsets() {
        
        //auto float_rand = std::uniform_real_distribution<float>(-offset_factor/2.0f, offset_factor / 2.0f);
        
        auto float_rand = std::normal_distribution<float>(0.0f, 0.3f);
        auto anomaly_rand = std::uniform_int_distribution<int>(0, 100000);
        auto anomaly_rand2 = std::uniform_int_distribution<int>(-200, 200);
        
        float sum = float_rand(gen);
        _line_offsets_host[0] = 0.75 * _line_offsets_host[_height - 1] + sum * offset_factor;
        
        for (int i = 1; i < _height; ++i) {
            sum += float_rand(gen);
            float last_offset = _line_offsets_host[i];
            _line_offsets_host[i] =  _line_offsets_host[i - 1] + sin(sum);
            //_line_offsets_host[i] = (sum);
            
            int resolve_speed = 20;
            
            if (_line_offset_linger[i] > 0) {
                _line_offset_linger_next[i+1] = _line_offset_linger[i] - 1;
                
                _line_offsets_host[i] += _line_offset_rand[i]
                * ((0.8 +
                   sin(
                       (
                            (
                             resolve_speed
                             - std::abs(_line_offset_linger[i])
                             ) / (float)resolve_speed
                        ) * M_PI * 1.705f
                       )) / 1.8f
                   );
                /*
                if (i == 50) {
                    std::stringstream strm;
                    auto linger = _line_offset_linger[i];
                    strm << "sin: " << sin(
                                          (
                                           (
                                            resolve_speed
                                            - std::abs(_line_offset_linger[i])
                                            ) / (float)resolve_speed
                                           ) * M_PI * 1.705f
                                          );
                    putLog(strm.str());
                    
                }*/
                
            }
            
            /*
            if (i == 50) {
                usleep(10);
            }
             */
            
            auto check = anomaly_rand(gen);
            if (check < 20) {
                _line_offset_rand[i] = anomaly_rand2(gen);
                //_line_offsets_host[i] += _line_offset_rand[i];
                _line_offset_linger_next[i] = resolve_speed;
            }
            
            if (check == 0) {
                _line_offset_rand[i] = anomaly_rand(gen);
                _line_offset_linger_next[i] = resolve_speed;
            }
            
            
            _line_offsets_host[i] = (0.1 * last_offset + 0.85 * _line_offsets_host[i]);
        }
        
        auto copy = _line_offsets_host;
        for (int i = 0; i < _height; ++i) {
            
            float prev_2 = 0.05 * copy[std::max(i - 2, 0)];
            float prev_1 = 0.25 * copy[std::max(i - 1, 0)];
            
            float now_1 = 0.4 * copy[i];
            
            float next_1 = 0.25 * copy[std::min<int>(i + 1, _height - 1)];
            float next_2 = 0.05 * copy[std::min<int>(i + 2, _height - 1)];
            
            _line_offsets_host[i] = (prev_2 + prev_1 + now_1 + next_1 + next_2);
        }
        
        _line_offset_linger = _line_offset_linger_next;
        for (auto& a: _line_offset_linger_next) {
            a = 0;
        }
        cl::Event event;
        queue->enqueueWriteBuffer(*_line_offsets, CL_TRUE, 0, _height * sizeof(cl_float), _line_offsets_host, NULL, &event);
        event.wait();
        
        
    }
    
    void tapeProcess::adjust_alignment() {
        //if (align_tick % 10 == 0) {
            auto float_rand = std::normal_distribution<float>(-0.005f, 0.005f);
            
            tape_align_red = 0.00f;
            tape_align_green += float_rand(gen);
            tape_align_blue += float_rand(gen);
            
        //}resize_out
        tape_align_green = 0.6 * tape_align_green;
        tape_align_blue = 0.6 * tape_align_blue;
        
        align_tick++;
    }
    
}
