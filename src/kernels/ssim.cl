#ifndef media_encoder_ssim_cl
#define media_encoder_ssim_cl


float4 bt709yuvrgb(float4 input);
float4 bt709rgbyuv(float4 input);

float puEncode(float in, __global float * pu_map);

float gaussian_parameter(float scale, float x, float y, float x0, float y0, float dev_x, float dev_y) {
    scale = 1.0f / (2.0f * M_PI * pow(dev_x, 2.0f));
    const float x_term = (pow(x - x0, 2.0f))/(2*pow(dev_x, 2.0f));
    const float y_term = (pow(y - y0, 2.0f))/(2*pow(dev_y, 2.0f));
    return scale * exp(-(x_term + y_term));
}

__kernel void pu_encode(read_only image2d_t input, write_only image2d_t output, __global float * pu_map) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE| CLK_ADDRESS_CLAMP_TO_EDGE;
    const float in = puEncode(bt709rgbyuv(read_imagef(input, sampler_nearest, (int2)(get_global_id(0), get_global_id(1)))).x, pu_map);
    
    write_imagef(output, (int2)(get_global_id(0), get_global_id(1)), (float4)(in, in, in, 1.0f));
}

__kernel void ssim_process(read_only image2d_t original, read_only image2d_t input, write_only image2d_t output, __global float * output_array, int half_size, const float max_value, float range) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE| CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(output);
    
    const int2 coord = (int2)(get_global_id(0)+5, get_global_id(1)+5);
    
	//const float max_value = 1e4;
    
    const float in = min(read_imagef(input, sampler_nearest, coord).x, max_value);
    const float orig = min(read_imagef(original, sampler_nearest, coord).x, max_value);
    
    float in_mean = 0.0f;
    float orig_mean = 0.0f;
    
    float gauss_sum = 0.0f;
    
    for (int j = -half_size; j <= half_size; ++j) {
        for (int i = -half_size; i <= half_size; ++i) {
            const float in_a = min(read_imagef(input, sampler_nearest, coord + (int2)(i, j)).x, max_value);
            const float orig_a = min(read_imagef(original, sampler_nearest, coord + (int2)(i, j)).x, max_value);
            const float gauss = gaussian_parameter(1.0f, i, j, 0.0f, 0.0f, 1.5f, 1.5f);
            gauss_sum += gauss;
            in_mean += gauss * in_a;
            orig_mean += gauss * orig_a;
        }
    }
    
    in_mean = in_mean / gauss_sum;
    orig_mean = orig_mean / gauss_sum;
    
    float in_var = 0.0f;
    float orig_var = 0.0f;
    float co_var = 0.0f;
    /*
    for (int j = -half_size; j <= half_size; ++j) {
        for (int i = -half_size; i <= half_size; ++i) {
            const float in_a = bt709rgbyuv(read_imagef(input, sampler_nearest, coord + (int2)(i, j))).x;
            const float orig_a = bt709rgbyuv(read_imagef(original, sampler_nearest, coord + (int2)(i, j))).x;
            const float gauss = gaussian_parameter(1.0f, i, j, 0.0f, 0.0f, 1.5f, 1.5f);
            in_var += gauss * pow((in_a - in_mean), 2.0f);
            orig_var += gauss * pow((orig_a - orig_mean), 2.0f);
            
            co_var += gauss * (in_a - in_mean) * (orig_a - orig_mean);
        }
    }
    
    in_var = in_var / gauss_sum;
    orig_var = orig_var / gauss_sum;
    co_var = co_var / gauss_sum;
    
    //in_var = sqrt(in_var);
    //orig_var = sqrt(orig_var);
     */
    
    
    for (int j = -half_size; j <= half_size; ++j) {
        for (int i = -half_size; i <= half_size; ++i) {
            const float in_a = min(read_imagef(input, sampler_nearest, coord + (int2)(i, j)).x, max_value);
            const float orig_a = min(read_imagef(original, sampler_nearest, coord + (int2)(i, j)).x, max_value);
            const float gauss = gaussian_parameter(1.0f, i, j, 0.0f, 0.0f, 1.5f, 1.5f);
            in_var += gauss * in_a * in_a;
            orig_var += gauss * orig_a * orig_a;
            
            co_var += gauss * (in_a) * (orig_a);
        }
    }
    /*
    in_var = (in_var - in_mean)/ gauss_sum;
    orig_var = (orig_var - orig_mean) / gauss_sum;
    co_var = co_var - (in_mean * orig_mean) / gauss_sum;
    */
    
    in_var = (in_var/gauss_sum) - (in_mean * in_mean);
    orig_var = (orig_var/gauss_sum) - (orig_mean * orig_mean);
    co_var = (co_var / gauss_sum)  - (in_mean * orig_mean);
    
    
    //in_var = sqrt(in_var);
    //orig_var = sqrt(orig_var);
    
    
    const float L = range; // ssim dynamic range
    
    const float k1 = 0.01f;
    const float k2 = 0.03f;
    
    const float c1 = pow(k1 * L, 2.0f);
    const float c2 = pow(k2 * L, 2.0f);
    
    //const float ssim = (2*in_mean*orig_mean + c1) * (2*co_var + c2)
    /// ( (pow(in_mean, 2.0f) + pow(orig_mean, 2.0f) + c1) * (pow(in_var, 2.0f) + pow(orig_var, 2.0f) + c2) );
    
    const float ssim = (2.0f*in_mean*orig_mean + c1) * (2.0f*co_var + c2)
    / ( (pow(in_mean, 2.0f) + pow(orig_mean, 2.0f) + c1) * (in_var + orig_var + c2) );
    
    
    write_imagef(output, (int2)(get_global_id(0), get_global_id(1)), (float4)(ssim, ssim, ssim, 1.0f));
    
    output_array[get_global_id(1) * (dim.x) + get_global_id(0)] = ssim;
}


#endif
