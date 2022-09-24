#ifndef SOBEL_CL
#define SOBEL_CL

__kernel void sobel_filter(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    float in[3][3];
    
    float grad_x, grad_y;
    
    for (int j = -1; j < 2; ++j) {
        for (int i = -1; i < 2; ++i) {
            in[j+1][i+1] = bt709rgbyuv(read_imagef(input, sampler, coord + (int2)(i,j))).x;
        }
    }
    
    grad_x = -in[0][0] - 2.0f * in[1][0] - in[2][0] + in[0][2] + 2.0f * in[1][2] + in[2][2];
    
    grad_y = -in[0][0] - 2.0f * in[0][1] - in[0][2] + in[2][0] + 2.0f * in[2][1] + in[2][2];
    
    const float out = sqrt(pow(grad_x, 2.0f) + pow(grad_y, 2.0f));
    
    write_imagef(output, coord, (float4)(out, out, out, 1.0f));
}

#endif
