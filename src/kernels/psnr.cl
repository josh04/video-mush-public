//
//  psnr.cl
//  
//
//  Created by Josh McNamee on 14/07/2015.
//
//

#ifndef _psnr_cl
#define _psnr_cl

float4 bt709yuvrgb(float4 input);
float4 bt709rgbyuv(float4 input);

__kernel void log_squared_error(read_only image2d_t original, read_only image2d_t fraud, __global float * psnrs, const float max_value) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(original);
    const int x = get_global_id(0), y = get_global_id(1);

    const float inOrig = min(log(max(bt709rgbyuv(read_imagef(original, sampler, (int2)(x, y))).x, 1e-5f)), max_value);
    
    const float inFraud = min(log(max(bt709rgbyuv(read_imagef(fraud, sampler, (int2)(x, y))).x, 1e-5f)), max_value);
	//const float4 inFraud = log(max(min(read_imagef(fraud, sampler_linear, (float2)((x + 0.5f)/dim.x, (y + 0.5f)/dim.y)), max_value), 1e-5f));

	const float diff = (inOrig) - (inFraud);

	const float out = diff * diff;

    //const float out = (pow(bt709rgbyuv(inOrig).x - bt709rgbyuv(inFraud).x, 2.0f) +
    //                   pow(inOrig.y - inFraud.y, 2.0f) +
    //                   pow(inOrig.z - inFraud.z, 2.0f) ) / 3.0;
    
    const int regis = y * dim.x + x;
    psnrs[regis] = out;
}


__kernel void squared_error(read_only image2d_t original, read_only image2d_t fraud, __global float * psnrs, const float max_value) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 dim = get_image_dim(original);
    const int x = get_global_id(0), y = get_global_id(1);
    //const float4 inOrig = min(read_imagef(original, sampler, (int2)(x, y)), max_value);
    const float4 inOrig = min(read_imagef(original, sampler, (int2)(x, y)), max_value);

    //const float4 inFraud = min(read_imagef(fraud, sampler_linear, (float2)((x+0.5f)/(float)dim.x, (y + 0.5f)/(float)dim.y)), max_value);
    //const float4 inFraud = min(read_imagef(fraud, sampler, (int2)(x, y)), 1.0f);
    const float4 inFraud = min(read_imagef(fraud, sampler, (int2)(x, y)), max_value); // we have a max_value min to stop high values skewing. this breaks matlab compatibility.
	
	const float diff = bt709rgbyuv(inOrig).x - bt709rgbyuv(inFraud).x;

	const float out = diff * diff;
    //const float out = (pow(inOrig.x - inFraud.x, 2.0f) +
    //                   pow(inOrig.y - inFraud.y, 2.0f) +
    //                   pow(inOrig.z - inFraud.z, 2.0f) ) / 3.0;
    
    const int regis = y * dim.x + x;
    psnrs[regis] = out;
}

float interpolate_table(float in, __global float * pu_map) {
    int count = 0;
    float out = 0.0f;
    while (count < 4096) {
        if (pu_map[count*2] > in) {
            
            const float prev_first = pu_map[count*2 - 2];
            const float prev_second = pu_map[count*2 - 1];
            const float p_first = pu_map[count*2];
            const float p_second = pu_map[count*2+1];
            
            out = prev_second + (p_second - prev_second) * (in - prev_first) / (p_first - prev_first);
            break;
            
        }
        count++;
    }
    return out;
}

float puEncode(float in, __global float * pu_map) {
    const float l_min = 1e-5f;
    const float l_max = 1e10f;
    
    
    const float l = log10(max(min(in, l_max), l_min));
    
    const float pu_l = 31.9270f;
    const float pu_h = 149.9244f;
    
    const float final = 255.0f * (interpolate_table(l, pu_map) - pu_l) / (pu_h-pu_l);
    
    return final;
}

__kernel void squared_error_pu(read_only image2d_t original, read_only image2d_t fraud, __global float * psnrs, const float max_value, __global float * pu_map) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 dim = get_image_dim(original);
    const int x = get_global_id(0), y = get_global_id(1);
    
    const float inOrig = puEncode(bt709rgbyuv(read_imagef(original, sampler, (int2)(x, y))).x, pu_map);
    
    //const float4 inFraud = min(read_imagef(fraud, sampler_linear, (float2)((x+0.5f)/(float)dim.x, (y + 0.5f)/(float)dim.y)), max_value);
    const float inFraud = puEncode(bt709rgbyuv(read_imagef(fraud, sampler, (int2)(x, y))).x, pu_map);
    
    const float diff = inOrig - inFraud;
    
    const float out = diff * diff;
    //const float out = (pow(inOrig.x - inFraud.x, 2.0f) +
    //                   pow(inOrig.y - inFraud.y, 2.0f) +
    //                   pow(inOrig.z - inFraud.z, 2.0f) ) / 3.0;
    
    const int regis = y * dim.x + x;
    psnrs[regis] = out;
}



__kernel void psnr_diff(read_only image2d_t original, read_only image2d_t fraud, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 dim = get_image_dim(original);
    const int x = get_global_id(0), y = get_global_id(1);
    const float4 inOrig = read_imagef(original, sampler, (int2)(x*2, y*2));
    const float4 inFraud = read_imagef(fraud, sampler_linear, (float2)(x*2/(float)dim.x, y*2/(float)dim.y));
    
    float4 diff;// = fabs(bt709yuvrgb(inOrig).x - bt709yuvrgb(inFraud).x);
    float diff2 = fabs(bt709rgbyuv(inOrig).x - bt709rgbyuv(inFraud).x);//0.333f * diff.x + 0.333f * diff.y + 0.333f * diff.z;
    diff.s0 = diff2;
    diff.s1 = diff2;
    diff.s2 = diff2;
    diff.s3 = 1.0f;
    
    float diff3 = pow(diff2, 2.0f);
    write_imagef(output, (int2)(x,y), inOrig);
    write_imagef(output, (int2)(x,y+dim.y/2), inFraud);
    write_imagef(output, (int2)(x+dim.x/2,y), diff);
    write_imagef(output, (int2)(x+dim.x/2,y+dim.y/2), diff3);
}


__kernel void psnr_diff_mse(read_only image2d_t original, read_only image2d_t fraud, write_only image2d_t output) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	//const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
	const int2 dim = get_image_dim(original);
	const int x = get_global_id(0), y = get_global_id(1);
	const float4 inOrig = read_imagef(original, sampler, (int2)(x, y));
	const float4 inFraud = read_imagef(fraud, sampler, (int2)(x, y));

	float4 diff;// = fabs(bt709yuvrgb(inOrig).x - bt709yuvrgb(inFraud).x);
	float diff2 = fabs(bt709rgbyuv(inOrig).x - bt709rgbyuv(inFraud).x);//0.333f * diff.x + 0.333f * diff.y + 0.333f * diff.z;
	diff.s0 = diff2;
	diff.s1 = diff2;
	diff.s2 = diff2;
	diff.s3 = 1.0f;

	write_imagef(output, (int2)(x, y), diff);
}

__kernel void psnr_sum(__global float * mean_squared_errors, const int length, const int offset) {
	const int x = get_global_id(0);

	if (x + offset < length) {
		const float output = (mean_squared_errors[x] + mean_squared_errors[x + offset]);
		mean_squared_errors[x] = output;
	}

}

#endif
