#ifndef CLEAR_CL
#define CLEAR_CL

__kernel void clear_image(write_only image2d_t output) {
    
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const float4 paste = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    write_imagef(output, coord, paste);

}

#endif
