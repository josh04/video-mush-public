//
//  chromaSwap.cl
//  video-mush
//
//  Created by Josh McNamee on 24/03/2015.
//
//

#ifndef video_mush_chromaSwap_cl
#define video_mush_chromaSwap_cl

float4 bt709yuvrgb(float4 input);
float4 bt709rgbyuv(float4 input);

__kernel void chromaSwap(read_only image2d_t luma, read_only image2d_t chroma, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 in_l = bt709rgbyuv(read_imagef(luma, sampler, (int2)(x, y)));
    const float4 in_c = bt709rgbyuv(read_imagef(chroma, sampler, (int2)(x, y)));
    //   const float4 in_a = clamp(read_imagef(input, sampler, (int2)(x, y)), 0.0f, 1.0f);
    
    // const float4 out = read_imagef(cube, cubesampler, (float4)(in_a.z, in_a.y, in_a.x, 1.0f));
    //write_imagef(output, (int2)(x, y), (float4)(out.x, out.y, out.z, 1.0f));
    
    const float4 temp = bt709yuvrgb((float4)(in_l.x, in_c.y, in_c.z, 1.0f));
    
    write_imagef(output, (int2)(x, y), temp);
}

#endif
