//
//  gaussian.cl
//  video-mush
//
//  Created by Josh McNamee on 24/03/2015.
//
//

#ifndef video_mush_gaussian_cl
#define video_mush_gaussian_cl

__kernel void bayer_gaussian_vertical(read_only image2d_t input, write_only image2d_t output, float sigma, int half_window) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    int x = get_global_id(0)*2, y = get_global_id(1)*2;
    
    float4 out_r = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 out_g1 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 out_g2 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 out_b = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    float div = 0.0f;
    
    const float mult = 1.0f/sqrt(M_PI * 2.0f * pow(sigma, 2.0f));
    const float denom = 2*pow(sigma, 2.0f);
    
    for (int j = -half_window; j < half_window; ++j) {
        const float4 in_r = log(read_imagef(input, sampler, (int2)(x, y + j*2)));
        const float4 in_g1 = log(read_imagef(input, sampler, (int2)(x+1, y + j*2)));
        const float4 in_g2 = log(read_imagef(input, sampler, (int2)(x, y+1 + j*2)));
        const float4 in_b = log(read_imagef(input, sampler, (int2)(x+1, y+1 + j*2)));
        
        const float gauss = mult * exp(-(pow(j, 2.0f) / denom));
        
        div += gauss;
        
        out_r += gauss * in_r;
        out_g1 += gauss * in_g1;
        out_g2 += gauss * in_g2;
        out_b += gauss * in_b;
    }
    
    out_r = exp(out_r / div);
    out_g1 = exp(out_g1 / div);
    out_g2 = exp(out_g2 / div);
    out_b = exp(out_b / div);
    
    write_imagef(output, (int2)(x, y), out_r);
    write_imagef(output, (int2)(x+1, y), out_g1);
    write_imagef(output, (int2)(x, y+1), out_g2);
    write_imagef(output, (int2)(x+1, y+1), out_b);
}


__kernel void bayer_gaussian_horizontal(read_only image2d_t input, write_only image2d_t output, float sigma, int half_window) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    int x = get_global_id(0)*2, y = get_global_id(1)*2;
    
    float4 out_r = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 out_g1 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 out_g2 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 out_b = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    float div = 0.0f;
    
    const float mult = 1.0f/sqrt(M_PI * 2.0f * pow(sigma, 2.0f));
    const float denom = 2*pow(sigma, 2.0f);
    
    for (int j = -half_window; j < half_window; ++j) {
        const float4 in_r = log(read_imagef(input, sampler, (int2)(x + j*2, y)));
        const float4 in_g1 = log(read_imagef(input, sampler, (int2)(x+1 + j*2, y)));
        const float4 in_g2 = log(read_imagef(input, sampler, (int2)(x + j*2, y+1)));
        const float4 in_b = log(read_imagef(input, sampler, (int2)(x+1 + j*2, y+1)));
        
        const float gauss = mult * exp(-(pow(j, 2.0f) / denom));
        
        div += gauss;
        
        out_r += gauss * in_r;
        out_g1 += gauss * in_g1;
        out_g2 += gauss * in_g2;
        out_b += gauss * in_b;
    }
    
    out_r = exp(out_r / div);
    out_g1 = exp(out_g1 / div);
    out_g2 = exp(out_g2 / div);
    out_b = exp(out_b / div);
    
    write_imagef(output, (int2)(x, y), out_r);
    write_imagef(output, (int2)(x+1, y), out_g1);
    write_imagef(output, (int2)(x, y+1), out_g2);
    write_imagef(output, (int2)(x+1, y+1), out_b);
}


__kernel void bayer_sharpen(read_only image2d_t input, write_only image2d_t output) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    int x = get_global_id(0)*2, y = get_global_id(1)*2;
    
    float4 out_r = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 out_g1 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 out_g2 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 out_b = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    //float div = 0.0f;
    
    float kern[9] = {-1.0f, -1.0f,  -1.0f,
                     -1.0f,  16.0f, -1.0f,
                     -1.0f, -1.0f,  -1.0f };
    
    for (int j = -1; j < 2; ++j) {
        for (int i = -1; i < 2; ++i) {
            
            const float4 in_r = read_imagef(input, sampler, (int2)(x + i*2, y + j*2));
            const float4 in_g1 = read_imagef(input, sampler, (int2)(x+1 + i*2, y + j*2));
            const float4 in_g2 = read_imagef(input, sampler, (int2)(x + i*2, y+1 + j*2));
            const float4 in_b = read_imagef(input, sampler, (int2)(x+1 + i*2, y+1 + j*2));
            
            const float sharp = kern[(3*(j+1)) + (i+1)];
            
            //div += gauss;
            
            out_r += sharp * in_r;
            out_g1 += sharp * in_g1;
            out_g2 += sharp * in_g2;
            out_b += sharp * in_b;
            
        }
    }
    
    float div = 8.0f;
    out_r = (out_r / div);
    out_g1 = (out_g1 / div);
    out_g2 = (out_g2 / div);
    out_b = (out_b / div);
    
    write_imagef(output, (int2)(x, y), out_r);
    //write_imagef(output, (int2)(x, y), read_imagef(input, sampler, (int2)(x, y)));
    write_imagef(output, (int2)(x+1, y), out_g1);
    write_imagef(output, (int2)(x, y+1), out_g2);
    write_imagef(output, (int2)(x+1, y+1), out_b);
    //write_imagef(output, (int2)(x+1, y+1), read_imagef(input, sampler, (int2)(x+1, y+1)));
}

#endif
