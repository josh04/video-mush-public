//
//  waveform.cl
//  video-mush
//
//  Created by Josh McNamee on 29/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_waveform_cl
#define media_encoder_waveform_cl

__kernel void waveform_clear(__global unsigned int * counts, write_only image2d_t image, int span) {
    
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    counts[coord.y*span + coord.x] = 0;
    write_imagef(image, coord, (float4)(0.0, 0.0, 0.0, 0.0));
}

__kernel void gamma(read_only image2d_t input, write_only image2d_t output, float gamma) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const float4 gamd = pow(read_imagef(input, sampler, coord), gamma);
    write_imagef(output, coord, gamd);
}

unsigned int _waveform_count(const unsigned int waveform_bar, const int luma_bins, const float out) {
    unsigned int loc;
    
    if (out > 1.0f) {
        const float logout = clamp(log2(out), 0.0f, 16.0f);
        unsigned int bin = (unsigned int)(logout*(unsigned int)(luma_bins*0.5f/16.0f));
        loc = waveform_bar*luma_bins+clamp((uint)((uint)(luma_bins*0.5f)+bin), (uint)0, (uint)(luma_bins-1));
    } else {
        loc = waveform_bar*luma_bins + clamp((uint)(luma_bins*0.5f*out), (uint)0, (uint)(luma_bins-1));
    }
    return loc;
}

__kernel void waveform_count_luma(read_only image2d_t input, __global unsigned int * counts, const int width_bins, const int luma_bins, const unsigned int ch, const float exfo) {
    
	const int2 size = get_image_dim(input);
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const float4 weight4 = (float4)(0.299f, 0.587f, 0.114f, 0.0f);
    
	const float out = dot(read_imagef(input, sampler, coord), weight4) * exfo;
    
    const unsigned int waveform_bar = (unsigned int)((float)coord.x * (width_bins/(float)size.x));
    
    atomic_inc(counts+_waveform_count(waveform_bar, luma_bins, out));
}


__kernel void waveform_count(read_only image2d_t input, __global unsigned int * counts, const int width_bins, const int luma_bins, const unsigned int ch, const float exponent) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
	const int2 size = get_image_dim(input);
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	
	const float4 color = read_imagef(input, sampler, coord);
    
    float out;
    if (ch == 0) {
        const float4 weight4 = (float4)(0.299f, 0.587f, 0.114f, 0.0f);
        out = dot(color, weight4);
	}
    
    if (ch == 1) {
        out = color.x;
	}
    
    if (ch == 2) {
        out = color.y;
	}
    
    if (ch == 3) {
        out = color.z;
	}
//    const float exponent = pow(2.0f, exfo);
    out = out * exponent;
        
    const unsigned int waveform_bar = (unsigned int)((float)coord.x * (width_bins/(float)size.x));
    
    unsigned int loc;
    
    if (out > 1.0f) {
        const float logout = clamp(log2(out), 0.0f, 16.0f);
        unsigned int bin = (unsigned int)(logout*(unsigned int)(luma_bins*0.5f/16.0f));
        loc = waveform_bar*luma_bins+clamp((uint)((uint)(luma_bins*0.5f)+bin), (uint)0, (uint)(luma_bins-1));
    } else {
        loc = waveform_bar*luma_bins + clamp((uint)(luma_bins*0.5f*out), (uint)0, (uint)(luma_bins-1));
    }

    atomic_inc(counts+loc);
}

__kernel void waveform_draw(__global unsigned int * counts, write_only image2d_t output, unsigned int max, unsigned int ch, unsigned int luma_bins) {
    int2 dim = get_image_dim(output);
    
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
//    const int im_height_start = dim.y - 40;
//    const int im_width_start = dim.x - 40 - dim.x/8;
    
    const int loc = coord.x*dim.y + coord.y;
    const float wave = sqrt((float)counts[loc] / (float)max);
    
    float4 out;
    if (ch == 0) {
        out = (float4)(wave, wave, wave, 0.9f);
	}
    
    if (ch == 1) {
        out = (float4)(wave, 0.0f, 0.0f, 0.9f);
	}
    
    if (ch == 2) {
        out = (float4)(0.0f, wave, 0.0f, 0.9f);
	}
    
    if (ch == 3) {
        out = (float4)(0.0f, 0.0f, wave, 0.9f);
	}
    /*
    if (coord.y == 63) {
        float4 temp = (float4)(1.0f, 0.0f, 1.0f, 0.4f) * 0.4f + out*out.s3*(1.0f-0.4f);
        float div = 0.4f+out.s3*(1-0.4f);
        out = temp/div;
    }
    */
    if (coord.y == (unsigned int)(luma_bins*0.5f) || ( ( (coord.y - (unsigned int)(luma_bins*0.5f)) % 32 == 0 ) && coord.y > (unsigned int)(luma_bins*0.5f) ) ) {
        const float lineAlpha = 0.4f;
        const float4 temp = (float4)(1.0f, 0.0f, 1.0f, lineAlpha) * 0.4f + out*out.s3*(1.0f-lineAlpha);
        const float div = lineAlpha+out.s3*(1-lineAlpha);
        out = temp/div;
    }
    
    write_imagef(output, (int2)(coord.x, dim.y - coord.y-1), (float4)(out.x, out.y, out.z, 1.0f));
    
}

__kernel void waveform_copy(read_only image2d_t waveform, read_only image2d_t input, write_only image2d_t output, int x, int y) {
    const sampler_t sampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
	const sampler_t sampler2 = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = (int2)(get_global_size(0), get_global_size(1));
    const float2 acc = (float2)(coord.x/(float)dim.x, coord.y/(float)dim.y);
    
    const int2 out_dim = get_image_dim(output);
    
    const float4 wave = read_imagef(waveform, sampler, acc);
    const int2 out = (int2)(x+coord.x, y - coord.y);
    
    const float4 col = read_imagef(input, sampler2, out);
    const float4 comb = wave*wave.s3 + col*(1-wave.s3);
    write_imagef(output, out, comb);
}

__kernel void waveform_combiner(read_only image2d_t waveform, write_only image2d_t output, unsigned int position) {
    const sampler_t sampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = (int2)(get_global_size(0), get_global_size(1));
    
    const float2 acc = (float2)(coord.x/(float)dim.x, coord.y/(float)dim.y);
    
    const float4 wave = read_imagef(waveform, sampler, acc);
    
    const int2 out = (int2)(coord.x + position*(dim.x), coord.y);
    
    write_imagef(output, out, wave);
}

#endif


