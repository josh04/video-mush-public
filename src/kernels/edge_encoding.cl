//
//  edge_encoding
//  video-mush
//
//  Created by Josh McNamee on 13/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_edge_encoding_cl
#define media_encoder_edge_encoding_cl

__kernel void edge_binary_map(read_only image2d_t input, __global uchar * output, int width) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(input);
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    if (coord.x < dim.x && coord.y < dim.y) {
        const float4 color = read_imagef(input, sampler, coord);
        
        //if (coord.y * width + coord.x < width * 720) {
            output[coord.y * width + coord.x] = min((int)round(color.y), 1);
        //}
    }
    
}

__kernel void regular_sample_expand(read_only image2d_t input, write_only image2d_t output) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(output);
    
    const int2 coord_s = (int2)(get_global_id(0), get_global_id(1));
    const int2 coord = (int2)(get_global_id(0)*32, get_global_id(1)*32);
    
    if (coord.x < dim.x && coord.y < dim.y) {
        const float4 color = read_imagef(input, sampler, coord_s);
        
        write_imagef(output, coord, (float4)(color.x, 1.0f, 0.0f, 0.0f));
    }
}

__kernel void diffuse_final_correct(read_only image2d_t input, write_only image2d_t output) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(output);
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
        const float4 color = read_imagef(input, sampler, coord);
        
        write_imagef(output, coord, (float4)(color.x, color.x, color.x, 1.0f));
}



#endif


