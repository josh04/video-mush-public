//
//  falseColour.cl
//  video-mush
//
//  Created by Josh McNamee on 13/01/2015.
//
//

#ifndef video_mush_falseColour_cl
#define video_mush_falseColour_cl

float4 hsvtorgb(const float4 in) {
    if (in.y == 0.0f) {
        return (float4)(in.z, in.z, in.z, 1.0f);
    }
    const float h = in.x / 60.0f;			// sector 0 to 5
    const int i = (int)floor( h );
    const float f = h - (float)i;			// factorial part of h
    const float p = in.z * ( 1.0f - in.y );
    const float q = in.z * ( 1.0f - in.y * f );
    const float t = in.z * ( 1.0f - in.y * ( 1.0f - f ) );
    float4 out = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    switch( i ) {
        case 0:
            out = (float4)(in.z, t, p, 1.0f);
            break;
        case 1:
            out = (float4)(q, in.z, p, 1.0f);
            break;
        case 2:
            out = (float4)(p, in.z, t, 1.0f);
            break;
        case 3:
            out = (float4)(p, q, in.z, 1.0f);
            break;
        case 4:
            out = (float4)(t, p, in.z, 1.0f);
            break;
        default:
            out = (float4)(in.z, p, q, 1.0f);
            break;
    }
    return out;
}

float sigmoidFC(const float in, const float hmean) {
    const float tempY = 0.18f * in / hmean;
    const float sig = tempY / (tempY + 1.0);
    return sig;
}

__kernel void falseColour(read_only image2d_t input, write_only image2d_t output, const float hmean) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    float4 in = read_imagef(input, sampler, (int2)(x, y));
    
    in = bt709rgbyuv(in);
    
    const float sigY = sigmoidFC(in.x, hmean);
    
    const float4 out = hsvtorgb((float4)(240.0f * (1.0f - sigY), 1.0f, 1.0f, 1.0f));
    
    write_imagef(output, (int2)(x, y), out);
}

float4 bandingFalseColour(uint in) {
    const ushort rem = in % 6;
    float4 col = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    switch (rem) {
        case 0:
            col = (float4)(1.0f, 0.0f, 0.0f, 1.0f);
            break;
        case 1:
            col = (float4)(0.0f, 1.0f, 0.0f, 1.0f);
            break;
        case 2:
            col = (float4)(1.0f, 0.5f, 0.0f, 1.0f);
            break;
        case 3:
            col = (float4)(0.0f, 0.0f, 1.0f, 1.0f);
            break;
        case 4:
            col = (float4)(1.0f, 1.0f, 0.0f, 1.0f);
            break;
        case 5:
            col = (float4)(1.0f, 0.0f, 1.0f, 1.0f);
            break;
    }
    
    return col;
}

__kernel void shortFalseColour(__global ushort * input, write_only image2d_t output) {
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(output);
    
    //float in = (float)input[y*dim.x + x];// yuvMax;
    ushort inS = input[y*dim.x + x];
    
    float4 col = bandingFalseColour((uint)inS);
    
    write_imagef(output, (int2)(x, y), col);
}

__kernel void charFalseColour(__global uchar * input, write_only image2d_t output) {
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(output);
    
    //float in = (float)input[y*dim.x + x];// yuvMax;
    uchar inS = input[y*dim.x + x];
    
    float4 col = bandingFalseColour((uint)inS);
    
    write_imagef(output, (int2)(x, y), col);
}

#endif
