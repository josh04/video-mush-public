//
//  resize
//  video-mush
//
//  Created by Josh McNamee on 15/04/2016.
//
//

#ifndef resize_cl
#define resize_cl

__kernel void resize(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    const int2 dim = get_image_dim(output);
    
    const float i_x = x/(float)dim.x;
    const float i_y = y/(float)dim.y;
    
    const float4 in = read_imagef(input, sampler, (float2)(i_x, i_y));
    
    write_imagef(output, (int2)(x, y), in);
}

#endif
