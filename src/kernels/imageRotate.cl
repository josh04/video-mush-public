//
//  imageRotate.cl
//  video-mush
//
//  Created by Josh McNamee on 24/05/2015.
//
//

#ifndef video_mush_imageRotate_cl
#define video_mush_imageRotate_cl

float2 rotateFloat2(const float2 in, const float time, const float ratio) {
    const float c = cos(time*M_PI / 180.0f);
    const float s = sin(time*M_PI / 180.0f);
    
    const float2 mid_vec = (in - (float2)(0.5, 0.5)) * (float2)(2.0f, 2.0f);
    
	const float2 fin_vec = (float2)((mid_vec.x * c - mid_vec.y * s), (mid_vec.x * s + mid_vec.y * c) / ratio) / (float2)(2.0f, 2.0f) + (float2)(0.5, 0.5);
    
    return fin_vec;
}

__kernel void rotateImage(read_only image2d_t input, write_only image2d_t output, const float time) {
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP;
    
    const int x = get_global_id(0), y = get_global_id(1);
    const int2 dim_i = get_image_dim(output);
    const float2 dim = (float2)((float)dim_i.x, (float)dim_i.y);
    
    const float ratio = dim.y / dim.x;
    
    const float diff = ((dim.x - dim.y) / 2.0)/dim.x;
    const float2 coords = (float2)((float)x/dim.x, ((float)y/dim.x + diff));

    const float2 mid_vec = rotateFloat2(coords, time, ratio);
    
    const float4 in = read_imagef(input, sampler_linear, mid_vec);
    
    write_imagef(output, (int2)(x, y), in);
}

__kernel void imageToBuffer(read_only image2d_t input, __global float4 * output) {
    const sampler_t sampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP;
    
    const int x = get_global_id(0), y = get_global_id(1);
    const int2 dim = get_image_dim(input);
    
    output[x + dim.x*y] = read_imagef(input, sampler, (int2)(x,y));
}

__kernel void bufferToImage(__global float4 * input, write_only image2d_t output) {
    const int x = get_global_id(0), y = get_global_id(1);
    const int2 dim = get_image_dim(output);
    float4 in = input[x + dim.x*y];
    in = in/in.w;
    write_imagef(output, (int2)(x,y), in);
}

#endif
