//
//  chromaSwap.cl
//  video-mush
//
//  Created by Josh McNamee on 24/03/2015.
//
//

#ifndef video_mush_demoMode_cl
#define video_mush_demoMode_cl


__kernel void demo_grid(read_only image2d_t input, write_only image2d_t output, int grid_x, int grid_y, const float4 border_color) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    int x = get_global_id(0), y = get_global_id(1);
    
    const int2 dim = get_image_dim(output);
    
    float4 in = read_imagef(input, sampler, (float2)(x*3/(float)dim.x, y*3/(float)dim.y));
    
    const int2 coords_out = (int2)((x) + (dim.x / 3) * grid_x, (y) + (dim.y/3) * grid_y);
    
    if (border_color.w > 0.0f && (x < 10 || y < 10 || dim.x/3 - x < 10 || dim.y/3 - y < 10)) {
        in = border_color;
    }
    
    write_imagef(output, coords_out, in);
}

#endif
