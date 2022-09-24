//
//  PTF4.cl
//  video-mush
//
//  Created by Jonathan Hatchett on 3/24/15.
//
//

#ifndef video_mush_PTF4_cl
#define video_mush_PTF4_cl

float4 bt709yuvrgb(float4 input);

__kernel void encodePTF4(read_only image2d_t input, write_only image2d_t output, const float yuvMax) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);
	
	const float4 in_a = read_imagef(input, sampler, (int2)(x, y)) * pow(2.0f, -4.0f);
 //   const float4 in_a = clamp(read_imagef(input, sampler, (int2)(x, y)), 0.0f, 1.0f);
	
	// const float4 out = read_imagef(cube, cubesampler, (float4)(in_a.z, in_a.y, in_a.x, 1.0f));
	//write_imagef(output, (int2)(x, y), (float4)(out.x, out.y, out.z, 1.0f));
	
	float4 temp = pow(in_a / (float4)(yuvMax, yuvMax, yuvMax, 1.0), 1.0 / 4.0);
	
    //float4 temp = sqrt(sqrt(in_a / (float4)(yuvMax, yuvMax, yuvMax, 1.0)));
    
	write_imagef(output, (int2)(x, y), temp);
}

__kernel void decodePTF4(read_only image2d_t input, write_only image2d_t output, const float yuvMax) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 in_a = read_imagef(input, sampler, (int2)(x, y)) * pow(2.0f, 4.0f);
    //   const float4 in_a = clamp(read_imagef(input, sampler, (int2)(x, y)), 0.0f, 1.0f);
    
    // const float4 out = read_imagef(cube, cubesampler, (float4)(in_a.z, in_a.y, in_a.x, 1.0f));
    //write_imagef(output, (int2)(x, y), (float4)(out.x, out.y, out.z, 1.0f));
    
    float4 temp = pow(in_a, 4.0) * (float4)(yuvMax, yuvMax, yuvMax, 1.0);
    //float4 temp = pown(in_a, 8) * (float4)(yuvMax, yuvMax, yuvMax, 1.0);
    
    write_imagef(output, (int2)(x, y), temp);
}

float PTF4Forward(float x) {
	return pow((float)x, 8.0f);
}

float4 _decodePTF4(float in, const float inU, const float inV, const int bitDepth) {
	
	const float bitValues = pow(2.0f, bitDepth) - 1.0f;
	
	const float footRoom = pow(2.0f, bitDepth - 4); // divide by 16;
	const float headRoom = pow(2.0f, bitDepth - 8)*(20.0f); // divide by 256
	
	const float validValues = bitValues - footRoom - headRoom;
	
	const float validChromaValues = bitValues - footRoom - footRoom;
	
	in = (in - footRoom) / validValues;
	in = clamp(in, 0.0f, 1.0f);
	
	const float oU = (inU - footRoom) / validChromaValues;
	const float oV = (inV - footRoom) / validChromaValues;
	
	float4 rgb = bt709yuvrgb((float4)(in, oU, oV, 1.0f));
	rgb.x = PTF4Forward(rgb.x);
	rgb.y = PTF4Forward(rgb.y);
	rgb.z = PTF4Forward(rgb.z);
	
	return rgb;
}

__kernel void decodePTF4Char(__global uchar * input, write_only image2d_t output, const float yuvMax, int bitDepth) {
	int x = get_global_id(0), y = get_global_id(1);
	int2 dim = get_image_dim(output);
	
	const float in = (float)input[y*dim.x + x];// yuvMax;
	const float inU = (float)input[(dim.x*dim.y) + (int)(dim.x/2)*y + (int)(x/2)];
	const float inV = (float)input[(int)(dim.x*dim.y*1.5) + (int)(dim.x/2)*y + (int)(x/2)];
	const float4 out = _decodePTF4(in, inU, inV, bitDepth);
	
	write_imagef(output, (int2)(x, y), out*yuvMax);
}

__kernel void decodePTF4Short(__global ushort * input, write_only image2d_t output, const float yuvMax, int bitDepth) {
	int x = get_global_id(0), y = get_global_id(1);
	int2 dim = get_image_dim(output);
	
	const float in = (float)input[y*dim.x + x];// yuvMax;
	const float inU = (float)input[(dim.x*dim.y) + (int)(dim.x/2)*y + (int)(x/2)];
	const float inV = (float)input[(int)(dim.x*dim.y*1.5) + (int)(dim.x/2)*y + (int)(x/2)];
	const float4 out = _decodePTF4(in, inU, inV, bitDepth);
	
	write_imagef(output, (int2)(x, y), out*yuvMax);
}

#endif
