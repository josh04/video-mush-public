//
//  clamp.cl
//  video-mush
//
//  Created by Josh McNamee on 12/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_clamp_cl
#define media_encoder_clamp_cl


__kernel void clampFloatToBGR24(read_only image2d_t input, __global uchar * output, const float gamma) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);
	int2 dim = get_image_dim(input);
	float4 in = read_imagef(input, sampler, (int2)(x, y));
	in = clamp(in, 0.0f, 1.0f);
	in = pow(in, gamma);
	in = clamp(in, 0.0f, 1.0f);
	output[(dim.x*y + x) * 3] = (uchar)(in.s2*255.0f);
	output[(dim.x*y + x) * 3 + 1] = (uchar)(in.s1*255.0f);
	output[(dim.x*y + x) * 3 + 2] = (uchar)(in.s0*255.0f);
}
// from http://uk.mathworks.com/matlabcentral/fileexchange/36417-yuv-files-reading-and-converting/content/YUV/yuv2rgb.m
float4 bt709rgbyuv(const float4 input) {
    
    const float y = input.x*0.2126 +    input.y*0.7152 +    input.z*0.0722;
    const float u = input.x*-0.1146 +   input.y*-0.3854 +   input.z*0.5;
    const float v = input.x*0.5 +       input.y*-0.4542 +   input.z*-0.0458;
     
/*    const float y = input.x*0.299 +    input.y*0.587 +    input.z*0.114;
    const float u = input.x*-0.169 +   input.y*-0.331 +   input.z*0.5;
    const float v = input.x*0.5 +       input.y*-0.419 +   input.z*-0.081;*/
    return (float4)(y, (u*1.023f)+0.5f, (v*1.023f)+0.5f, 1.0f);
    //return (float4)(y, (u)+0.5f, (v)+0.5f, 1.0f);

}

float4 bt709yuvrgb(float4 input) {
    input.y = input.y - 0.5f;
    input.z = input.z - 0.5f;
    const float r = input.x*1 + input.y*0 +         input.z*1.570;
    const float g = input.x*1 + input.y*-0.187 +    input.z*-0.467;
    const float b = input.x*1 + input.y*1.856 +     input.z*0;
    return (float4)(r, g, b, 1.0f);
}

float _linearToSRGB(const float input) {
    const float a = 0.055f;
    if (input > 0.0031308) {
        return (1 + a) * pow(input, 1.0f / 2.4f) - a;
    } else {
        return 12.92 * input;
    }
    return pow(input, 0.4545f);
}

float _SRGBtoLinear(const float input) {
    const float a = 0.055f;
    if (input > 0.04045) {
        return pow((input + a)/(1+a), 2.4f);
    } else {
        return input/12.92;
    }
}

float4 linearToSRGB(const float4 input) {
    return (float4)(_linearToSRGB(input.x), _linearToSRGB(input.y), _linearToSRGB(input.z), input.w);
}


float4 SRGBtoLinear(const float4 input) {
    return (float4)(_SRGBtoLinear(input.x), _SRGBtoLinear(input.y), _SRGBtoLinear(input.z), input.w);
}

__kernel void encodeSRGB(read_only image2d_t input, write_only image2d_t output) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coords = (int2)(get_global_id(0), get_global_id(1));
    
    const float4 in = linearToSRGB(read_imagef(input, sampler, coords));
    write_imagef(output, coords, in);
}

__kernel void decodeSRGB(read_only image2d_t input, write_only image2d_t output) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coords = (int2)(get_global_id(0), get_global_id(1));
    
    const float4 in = SRGBtoLinear(read_imagef(input, sampler, coords));
    write_imagef(output, coords, in);
}


int4 _clampFloattoYUV(float4 in, float4 in2, int bitDepth, float yuvMax) {
    
    in = in / yuvMax;
    in2 = in2 / yuvMax;
    
    in = bt709rgbyuv(clamp(in, 0.0f, 1.0f));
    in2 = bt709rgbyuv(clamp(in2, 0.0f, 1.0f));
    
        
    const float bitValues = pow(2.0f, bitDepth) - 1.0f;
    
    const float footRoom = pow(2.0f, bitDepth - 4); // divide by 16;
    const float headRoom = pow(2.0f, bitDepth - 8)*(21.0f); // divide by 256
    
    const float validValues = bitValues - footRoom - headRoom;
    const float validChromaValues = bitValues - footRoom - footRoom;
    
    in.s1 = in.s1 - 0.5f;
    in.s2 = in.s2 - 0.5f;
    in2.s1 = in2.s1 - 0.5f;
    in2.s2 = in2.s2 - 0.5f;
    
    
    return (int4)((in.s0*validValues+footRoom),
                  (in2.s0*validValues+footRoom),
                  ((in.s1 + in2.s1) / 2.0f)*validChromaValues+pow(2.0f, bitDepth - 1) ,
                  ((in.s2 + in2.s2) / 2.0f)*validChromaValues+pow(2.0f, bitDepth - 1) );
    //temp 10-bit
    
}

float rec709backward(float in);
float rec709forward(float in);
float4 rec709backward4(float4 in_a);
float4 rec709forward4(float4 in_a);

__kernel void fix_rgb(read_only image2d_t input, write_only image2d_t output) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(input);
    
    
    float4 in = read_imagef(input, sampler, (int2)(x, y));
    //in = (in - (16.0f/255.0f)) * (255.0f/219.0f);
	//in = rec709backward4(in);
    
    write_imagef(output, (int2)(x, y), in);
}

int4 temp_clampFloattoYUV(float4 in, float4 in2, int bitDepth) {
    
   // in = bt709rgbyuv(clamp(rec709forward4(in) * (219.0f / 255.0f) + (16.0f / 255.0f), 0.0f, 1.0f));
   // in2 = bt709rgbyuv(clamp(rec709forward4(in2) * (219.0f / 255.0f) + (16.0f / 255.0f), 0.0f, 1.0f));
	//in = bt709rgbyuv(clamp(rec709forward4(in), 0.0f, 1.0f));
	//in2 = bt709rgbyuv(clamp(rec709forward4(in2), 0.0f, 1.0f));
	in = bt709rgbyuv(clamp(in, 0.0f, 1.0f));
	in2 = bt709rgbyuv(clamp(in2, 0.0f, 1.0f));

    
    const float bitValues = 255;
    
    const float footRoom = 16; // divide by 16;
    const float headRoom = 20; // divide by 256
    
    const float validValues = 220;
    const float validChromaValues = 224;
    
    in.s1 = in.s1 - 0.5f;
    in.s2 = in.s2 - 0.5f;
	in2.s1 = in2.s1 - 0.5f;
	in2.s2 = in2.s2 - 0.5f;
    
    return (int4)(round(in.s0*validValues+footRoom),
		round(in2.s0*validValues+footRoom),
                  //((in.s1 + in2.s1) / 2.0f)*validChromaValues+footRoom,
                  //((in.s2 + in2.s2) / 2.0f)*validChromaValues+footRoom);
		round(((in.s1 + in2.s1) / 2.0f)*validChromaValues+128),
			round(((in.s2 + in2.s2) / 2.0f)*validChromaValues+128));
    //temp 10-bit
    
}

__kernel void clampFloatToYUV422Short(read_only image2d_t input, __global  ushort * output, int bitDepth, float yuvMax) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(input);
    
    float4 in = read_imagef(input, sampler, (int2)(x*2, y));
    float4 in2 = read_imagef(input, sampler, (int2)(x*2+1, y));
    const int4 temp = _clampFloattoYUV(in, in2, bitDepth, yuvMax);
    //temp 10-bit
    
    const int big = pown(2.0f, bitDepth) - 1.0f;
    
    output[(dim.x*y + x*2)] = (ushort)(clamp(temp.s0, 0, big));
    output[(dim.x*y + x*2+1)] = (ushort)(clamp(temp.s1, 0, big));

    output[(dim.x*dim.y) + (dim.x/2*y + x)] = (ushort)(clamp(temp.s2, 0, big));
    output[(int)(dim.x*dim.y*1.5) + dim.x/2*y + x] = (ushort)(clamp(temp.s3, 0, big));
}

__kernel void clampFloatToYUV422Char(read_only image2d_t input, __global uchar * output, int bitDepth, float yuvMax) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(input);
    
    float4 in = read_imagef(input, sampler, (int2)(x*2, y));
    float4 in2 = read_imagef(input, sampler, (int2)(x*2+1, y));
    const int4 temp = temp_clampFloattoYUV(in, in2, bitDepth);
    //temp 10-bit
    output[(dim.x*y + x*2)] = (uchar)(temp.s0);
    output[(dim.x*y + x*2+1)] = (uchar)(temp.s1);
    
    output[(dim.x*dim.y) + (dim.x/2*y + x)] = (uchar)(temp.s2);
    output[(int)(dim.x*dim.y*1.5) + dim.x/2*y + x] = (uchar)(temp.s3);
}

float4 bandingFalseColour(uint in);

__kernel void chroma422to420Char(__global uchar * input, __global uchar * output, int bitDepth, int width, int height) {

	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);

	uchar in = input[width * y * 2 + x * 2];
	uchar in2 = input[width * y * 2 + x * 2 + 1];
	uchar in3 = input[width * (y * 2 + 1) + x * 2];
	uchar in4 = input[width * (y * 2 + 1) + x * 2 + 1];

	output[width * y * 2 + x * 2] = in;
	output[width * y * 2 + x * 2 + 1] = in2;
	output[width * (y * 2 + 1) + x * 2] = in3;
	output[width * (y * 2 + 1) + x * 2 + 1] = in4;

	uchar cr1 = input[(width*height) + (width / 2) * (y * 2) + x];
	uchar cr2 = input[(width*height) + (width / 2) * ((y * 2) + 1) + x];
	uchar cb1 = input[(int)(width*height*1.5) + (width / 2) * (y * 2) + x];
	uchar cb2 = input[(int)(width*height*1.5) + (width / 2) * ((y * 2) + 1) + x];

	output[(width*height) + (width / 2) * y + x] = ((int)cr1 + cr2) / 2;
	output[(width*height) + (int)(width*height*0.25) +  (width / 2) * y + x] = ((int)cb1 + cb2) / 2;

}

#endif

