//
//  nuHDR.hpp
//  video-mush
//
//  Created by Josh McNamee on 22/01/2015.
//
//

#ifndef video_mush_nuHDR_hpp
#define video_mush_nuHDR_hpp

__kernel void nuHDR(read_only image2d_t original, read_only image2d_t luminance, write_only image2d_t output) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const float4 orig = read_imagef(original, sampler, coord);
    const float4 lum = read_imagef(luminance, sampler, coord);
    
    const float4 out = orig / (lum.x + 1);
    write_imagef(output, coord, out);
}

#endif
