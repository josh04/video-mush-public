#ifndef media_encoder_scale_image_cl
#define media_encoder_scale_image_cl




__kernel void scale_image(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(output);
    const float2 coord = (float2)((get_global_id(0) + 0.5f)/dim.x, (get_global_id(1)+0.5f)/dim.y);
    
    const float4 in = read_imagef(input, sampler_linear, coord);
    
    write_imagef(output, (int2)(get_global_id(0), get_global_id(1)), (float4)(in.x, in.y, in.z, 1.0f));
}




#endif
