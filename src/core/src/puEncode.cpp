//
//  puEncode.cpp
//  mush-core
//
//  Created by Josh McNamee on 23/02/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#include <iostream>
#include <fstream>
#define NOMINMAX
#include <math.h>

#include "puEncode.hpp"
#include <azure/engine.hpp>
#include "opencl.hpp"
#include "mushLog.hpp"

puEncode::puEncode() {
    
}

puEncode::~puEncode() {
    
}

float puEncode::encode(float in) {
    
    
    float l_min = 1e-5f;
    float l_max = 1e10f;
    
    
    float l = log10(std::max(std::min(in, l_max), l_min));
    
    const float pu_l = 31.9270f;
    const float pu_h = 149.9244f;
    
    float out = 255.0f * (interpolate_table(l) - pu_l) / (pu_h-pu_l);
    
    return out;
}

void puEncode::build_table() {
    std::ifstream input;
#ifdef _WIN32
	auto path = azure::Engine::GetWindowInterface()->getResourcesFolder() / "resources/pu2_space.csv";
#else
	auto path = azure::Engine::GetWindowInterface()->getResourcesFolder() / "pu2_space.csv";
#endif
    input.open(path.generic_string());
    
    _lut.reserve(4096);
    
    std::string line;
	size_t count = 0;
    while (std::getline(input, line)) {
        _lut.push_back(std::pair<float, float>());
        auto& pair = _lut.back();
        sscanf(line.c_str(), "%f, %f", &pair.first, &pair.second);
    }

	if (_lut.size() < 4095) {
		putLog("Warning: pu lookup table ran short.");
	}
    
}

cl::Buffer * puEncode::get_table_buffer(std::shared_ptr<mush::opencl> context) {
    std::ifstream input;
#ifdef _WIN32
	auto path = azure::Engine::GetWindowInterface()->getResourcesFolder() / "resources/pu2_space.csv";
#else
	auto path = azure::Engine::GetWindowInterface()->getResourcesFolder() / "pu2_space.csv";
#endif
    input.open(path.generic_string());
    
    float * local_buffer = (float *)context->hostReadBuffer(4096 * 2 * sizeof(float), false);
    
    std::string line;
    size_t count = 0;
    while (std::getline(input, line)) {
        auto& first = local_buffer[count * 2];
        auto& second = local_buffer[count * 2 + 1];
        sscanf(line.c_str(), "%f, %f", &first, &second);
        count++;
    }

	if (count < 4096) {
		putLog("Warning: pu lookup table ran short.");
	}
    
    auto queue = context->getQueue();
    
    auto gpu_buffer = context->buffer(4096 * 2 * sizeof(float));
    
    cl::Event event;
    queue->enqueueWriteBuffer(*gpu_buffer, CL_TRUE, 0, 4096*2*sizeof(float), local_buffer, NULL, &event);
    event.wait();
    
    return gpu_buffer;
}

float puEncode::interpolate_table(float in) {
    
    if (_lut.size() == 0) {
        build_table();
    }
    
    float out = 0.0f;
    
    size_t count = 0;
    for (auto& p : _lut) {
        
        if (p.first > in) {
            auto& prev = _lut[count - 1];
            
            out = prev.second + (p.second - prev.second) * (in - prev.first) / (p.first - prev.first);
            break;
        }
        
        count++;
    }
    
    return out;
    
}

std::vector<std::pair<float, float>> puEncode::_lut;

/*
 function P = pu2_encode( L )
 % Perceptually uniform luminance encoding using the CSF from HDR-VDP-2
 %
 % P = pu2_encode( L )
 %
 % Transforms absolute luminance values L into approximately perceptually
 % uniform values P.
 %
 % This is meant to be used with display-referred quality metrics - the
 % image values much correspond to the luminance emitted from the target
 % HDR display.
 %
 % This is an improved encoding described in detail in the paper:
 %
 % Aydin, T. O., Mantiuk, R., & Seidel, H.-P. (2008). Extending quality
 % metrics to full luminance range images. Proceedings of SPIE (p. 68060B–10).
 % SPIE. doi:10.1117/12.765095
 %
 % Note that the P-values can be negative or greater than 255. Most metrics
 % can deal with such values.
 %
 % Copyright (c) 2014, Rafal Mantiuk <mantiuk@gmail.com>
 
 persistent P_lut;
 persistent l_lut;
 
 l_min = -5;
 l_max = 10;
 
 
 if( isempty( P_lut ) ) % caching for better performance
 
 metric_par.csf_sa = [30.162 4.0627 1.6596 0.2712];
 l_lut = linspace( l_min, l_max, 2^12 );
 S = hdrvdp_joint_rod_cone_sens( 10.^l_lut, metric_par );
 
 [~, P_lut] = build_jndspace_from_S(l_lut,S);
 end
 
 
 l = log10(max(min(L,10^l_max),10^l_min));
 
 pu_l = 31.9270;
 pu_h = 149.9244;
 
 P = 255 * (interp1( l_lut, P_lut, l ) - pu_l) / (pu_h-pu_l);
 
 end
 
 function S = hdrvdp_joint_rod_cone_sens( la, metric_par )
 % Copyright (c) 2011, Rafal Mantiuk <mantiuk@gmail.com>
 
 % Permission to use, copy, modify, and/or distribute this software for any
 % purpose with or without fee is hereby granted, provided that the above
 % copyright notice and this permission notice appear in all copies.
 %
 % THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 % WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 % MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 % ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 % WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 % ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 % OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 
 cvi_sens_drop = metric_par.csf_sa(2); % in the paper - p6
 cvi_trans_slope = metric_par.csf_sa(3); % in the paper - p7
 cvi_low_slope = metric_par.csf_sa(4); % in the paper - p8
 
 S = metric_par.csf_sa(1) * ( (cvi_sens_drop./la).^cvi_trans_slope+1).^-cvi_low_slope;
 
 end
 
 function [Y jnd] = build_jndspace_from_S(l,S)
 
 L = 10.^l;
 dL = zeros(size(L));
 
 for k=1:length(L)
 thr = L(k)/S(k);
 
 % Different than in the paper because integration is done in the log
 % domain - requires substitution with a Jacobian determinant
 dL(k) = 1/thr * L(k) * log(10);
 end
 
 Y = l;
 jnd = cumtrapz( l, dL );
 
 end
 
 
*/
