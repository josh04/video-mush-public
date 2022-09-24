//
//  displaceChannel.hpp
//  video-mush
//
//  Created by Josh McNamee on 03/05/2015.
//
//

#ifndef video_mush_displaceChannel_hpp
#define video_mush_displaceChannel_hpp

float4 bt709yuvrgb(float4 input);
float4 bt709rgbyuv(float4 input);

__kernel void displaceChannel(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 in = bt709rgbyuv(read_imagef(input, sampler, (int2)(x, y)));
    const float4 in2 = bt709rgbyuv(read_imagef(input, sampler, (int2)(x+10, y)));
    const float4 in3 = bt709rgbyuv(read_imagef(input, sampler, (int2)(x, y+10)));
    
    
    const float4 temp = bt709yuvrgb((float4)(in.x, in2.y, in3.z, 1.0f));
    
    write_imagef(output, (int2)(x, y), temp);
}

__kernel void downsample(read_only image2d_t input, write_only image2d_t output, uint samples_w, uint samples_h) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(output);
    
    float4 in = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float num = 0.0f;
    
    for (uint i = 0; i < samples_h; ++i) {
        for (uint j = 0; j < samples_w; ++j) {
            in += read_imagef(input, sampler, (int2)(x*samples_w + j,y*samples_h + i));
            num = num + 1.0f;
        }
    }
    
    in = in / num;
    
    write_imagef(output, (int2)(x, y), in);
}

__kernel void downsampleChromaRefit(read_only image2d_t luma, read_only image2d_t chroma, write_only image2d_t output, uint time, uint dist) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(output);
    
    float2 coords = (float2)((float)x/dim.x, (float)y/dim.y);
    
    const float4 l = bt709rgbyuv(read_imagef(luma, sampler, (int2)(x, y)));
    
    float shuffle = sin(100.0f*(float)time*M_PI/180.0f)*(float)dist;
    
    const float4 c = bt709rgbyuv(read_imagef(chroma, sampler_linear, coords+(float2)(shuffle/dim.x, shuffle/dim.y)));
    
    const float4 temp = bt709yuvrgb((float4)(l.x, c.y, c.z, 1.0f));
    
    write_imagef(output, (int2)(x, y), temp);
}

#endif
