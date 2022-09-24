//
//  barcode.cl
//  video-mush
//
//  Created by Josh McNamee on 24/02/2015.
//
//

#ifndef video_mush_barcode_cl
#define video_mush_barcode_cl


__kernel void barcode(read_only image2d_t input, write_only image2d_t output, unsigned int diff) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 in_a = read_imagef(input, sampler, (int2)(x, y));
    float4 in_b = read_imagef(input, sampler, (int2)(diff + x, y));
    
    in_b = (in_a + in_b) / 2.0f;
    
    write_imagef(output, (int2)(x, y), in_b);
}


__kernel void barcode_compress(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 in_a = read_imagef(input, sampler, (int2)(x, y));
    float4 in_b = read_imagef(input, sampler, (int2)(x+1, y));
    
    in_b = (in_a + in_b) / 2.0f;
    
    write_imagef(output, (int2)(x, y), in_b);
}


__kernel void barcode_copy_slice(read_only image2d_t input, write_only image2d_t output, int offset) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    write_imagef(output, (const int2)(get_global_id(0) + offset, get_global_id(1)), read_imagef(input, sampler, (const int2)(get_global_id(0), get_global_id(1))));

}

__kernel void barcode_copy_mix(read_only image2d_t input, read_only image2d_t existing, write_only image2d_t output, int offset, unsigned int turn) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 in_a = read_imagef(input, sampler, (int2)(x, y));
    float4 in_b = read_imagef(existing, sampler, (int2)(x+offset, y));
    
    in_b = (in_a + (in_b*turn)) / (turn+1);
    
    write_imagef(output, (int2)(x+offset, y), in_b);
}

#endif
