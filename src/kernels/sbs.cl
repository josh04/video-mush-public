#ifndef media_encoder_scale_sbs_cl
#define media_encoder_scale_sbs_cl




__kernel void sbs_pack(read_only image2d_t left, read_only image2d_t right, write_only image2d_t output) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(output);
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    float4 in = (float4)(1.0f, 0.0f, 0.0f, 1.0f);
    
    if (coord.x > dim.x/2.0f) {
        in = read_imagef(right, sampler_nearest, (int2)(coord.x - (dim.x/2.0f), coord.y));
    } else {
        in = read_imagef(left, sampler_nearest, (int2)(coord.x, coord.y));
    }
    
                         
    write_imagef(output, (int2)(get_global_id(0), get_global_id(1)), (float4)(in.x, in.y, in.z, 1.0f));
}



__kernel void sbs_unpack(read_only image2d_t in_image, uchar is_right, write_only image2d_t output) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(output);
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    float4 in = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    if (is_right > 0) {
        coord.x = coord.x + dim.x;
    }
    
        in = read_imagef(in_image, sampler_nearest, (int2)(coord.x, coord.y));
    
    
    write_imagef(output, (int2)(get_global_id(0), get_global_id(1)), (float4)(in.x, in.y, in.z, 1.0f));
}




#endif
