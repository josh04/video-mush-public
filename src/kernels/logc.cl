//
//  logc.cl
//  video-mush
//
//  Created by Josh McNamee on 28/02/2015.
//
//

#ifndef video_mush_logc_cl
#define video_mush_logc_cl

__constant static const float cut = 0.004201f;
__constant static const float a = 200.0f;
__constant static const float b = -0.729169f;
__constant static const float c = 0.247190f;
__constant static const float d = 0.385537f;
__constant static const float e = 193.235573f;
__constant static const float f = -0.662201f;

__constant static const float g = 0.149581642173f; // e * cut + f;

float logcdecode(float z) {
    float x;
    
    // After testing removing this branch using mix drops the framerate on my Macbook Pro 11,3 from ~95 to ~85fps @ 1920x1080
    if (z > g) {
        x = pow(10.0f, ((z - d) / c) - b) / a;
    } else {
        x = (z - f) / e;
    }
    
    return x;
}

float4 tfdecode(float4 rgb) {
    return (float4)(logcdecode(rgb.s0), logcdecode(rgb.s1), logcdecode(rgb.s2), rgb.s3);
}

float4 logcRGBtoLinearRGB(float4 rgb) {
    const float4 a = (float4)(1.617523,	-0.070573,	-0.021102, 0.0f);
    const float4 b = (float4)(-0.537287,	1.334613,	-0.226954, 0.0f);
    const float4 c = (float4)(-0.080237,	-0.26404,	1.248056, 0.0f);
    
    const float4 tmp = rgb*a+rgb*b+rgb*c;
    return (float4)(tmp.x, tmp.y, tmp.z, 1.0f);
}

__kernel void logC(read_only image2d_t input, write_only image2d_t output, read_only image3d_t cube) {
    const sampler_t cubesampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
       const float4 in_a = read_imagef(input, sampler, (int2)(x, y));
 //   const float4 in_a = clamp(read_imagef(input, sampler, (int2)(x, y)), 0.0f, 1.0f);
    
   // const float4 out = read_imagef(cube, cubesampler, (float4)(in_a.z, in_a.y, in_a.x, 1.0f));
    //write_imagef(output, (int2)(x, y), (float4)(out.x, out.y, out.z, 1.0f));
    
    float4 temp = tfdecode(in_a);
    
    temp = logcRGBtoLinearRGB(temp);
    
    write_imagef(output, (int2)(x, y), temp);
}

#endif
