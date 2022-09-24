//
//  exposure.cl
//  video-mush
//
//  Created by Josh McNamee on 13/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_exposure_cl
#define media_encoder_exposure_cl

__kernel void exposure(read_only image2d_t input, write_only image2d_t output, const float darken) {
    
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const float min = pow(2.0f, darken);
	
	const int2 size = get_image_dim(input);
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	
	if (coord.x < size.x && coord.y < size.y) {
        float4 color = read_imagef(input, sampler, coord);
        color = color * (float4)(min, min, min, 1.0f);
        write_imagef(output, coord, color);
	}
    
}

#endif


