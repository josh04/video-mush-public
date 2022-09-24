#ifndef FLIP_CL
#define FLIP_CL


__kernel void flip_vertical(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(output);
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 coord_out = (int2)(coord.x, dim.y - coord.y - 1);
    
    const float4 in = read_imagef(input, sampler_nearest, coord);
    
    write_imagef(output, coord_out, in);
}

#endif