#ifndef TAPE_CL
#define TAPE_CL

float4 bt709yuvrgb(float4 input);
float4 bt709rgbyuv(float4 input);


__kernel void tape_interlace_on(read_only image2d_t input, write_only image2d_t output) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(input);
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    int2 read_coord = (int2)(coord.x, coord.y / 2.0f);
    
    if (coord.y < dim.y / 2) {
        read_coord.y = coord.y * 2;
    } else {
        read_coord.y = (coord.y - dim.y / 2) * 2  + 1;
    }

    const float4 read = read_imagef(input, sampler, read_coord);
    write_imagef(output, coord, read);
}

__kernel void tape_interlace_off(read_only image2d_t input, write_only image2d_t output) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(input);
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    int2 read_coord = (int2)(coord.x, coord.y / 2.0f);
    
    if (coord.y % 2 == 1) {
        read_coord.y = read_coord.y + dim.y / 2.0f;
    }
    
    const float4 read = read_imagef(input, sampler, read_coord);
    write_imagef(output, coord, read);
}

__kernel void tape_convert(read_only image2d_t input, write_only image2d_t output) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 dim = get_image_dim(input);
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	
    const float4 color = bt709rgbyuv(read_imagef(input, sampler, coord));
    
    write_imagef(output, (int2)(coord.x, coord.y*7 + 0), (float4)(color.x, color.x, color.x, 1.0f));
    write_imagef(output, (int2)(coord.x, coord.y*7 + 1), (float4)(color.x, color.x, color.x, 1.0f));
    write_imagef(output, (int2)(coord.x, coord.y*7 + 2), (float4)(color.x, color.x, color.x, 1.0f));
    
    write_imagef(output, (int2)(coord.x, coord.y*7 + 3), (float4)(color.y, color.y, color.y, 1.0f));
    write_imagef(output, (int2)(coord.x, coord.y*7 + 4), (float4)(color.y, color.y, color.y, 1.0f));
    
    write_imagef(output, (int2)(coord.x, coord.y*7 + 5), (float4)(color.z, color.z, color.z, 1.0f));
    write_imagef(output, (int2)(coord.x, coord.y*7 + 6), (float4)(color.z, color.z, color.z, 1.0f));
}

float steer_tape_x(float coord) {
    if (coord > 1.0f) {
        coord = coord - 1.0f;
        return (steer_tape_x(coord));
    }
    if (coord < 0.0f) {
        coord = coord + 1.0f;
        return (steer_tape_x(coord));
    }
    return coord;
}

/*
 
 float2 tape_coord_to_image_coord(float adj_x, const float adj_y, const float2 dim) {
 adj_x = steer_tape_x(adj_x);
 int scaled_x = adj_x * dim.x * dim.y;
 int image_x = scaled_x % dim.x;
 int image_y = (scaled_x - image_x) / dim.x;
 
 float rescaled_x = image_x / (float)dim.x;
 float rescaled_y = ( image_y / (float)dim.y) + adj_y / (float)dim.y;
 
 return (float2)(rescaled_x, rescaled_y);
 //return (float2)(image_x / (float)dim.x, image_y / (float)dim.y);
 }
 */

float2 tape_coord_to_image_coord(float adj_x, const float adj_y, const float2 dim) {
    adj_x = steer_tape_x(adj_x);
    float scaled_x = adj_x * (dim.x) * dim.y;
    float image_x = (int)scaled_x % (int)(dim.x);
    float image_y = (scaled_x - image_x) / dim.x;
    image_x = scaled_x - image_y * dim.x;
    
    float rescaled_x = image_x / (dim.x);
    float rescaled_y = ( image_y / dim.y) + adj_y / dim.y;
    
    return (float2)(rescaled_x, rescaled_y);
    //return (float2)(image_x / (float)dim.x, image_y / (float)dim.y);
}

/*
float tape_xorshift_random(uint2 seed, int id) {
    ulong nseed = seed.x + seed.y << 32 + id;
    nseed = (nseed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
    uint result = nseed >> 16;
    
    return (float)result / INT_MAX;
}
 */

__kernel void tape_read(read_only image2d_t input, write_only image2d_t output, const float tape_align_master, const float tape_shift_master, const float4 tape_align, const float4 tape_shift, const float4 tape_shift_previous, float tape_shift_offset, __global float * _line_offsets) {
    
    const int2 dim = get_image_dim(output);
    
    
    
    const sampler_t sampler = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    
    const float2 f_dim_2 = (float2)((float)dim.x + tape_shift_master, (float)dim.y);
    
    float rand_shift = 0.0f;
    {
        float prev_2 = 0.05 *_line_offsets[max((int)get_global_id(1) - 2, 0)];
        float prev_1 = 0.25 * _line_offsets[max((int)get_global_id(1) - 1, 0)];
        
        float now_1 = 0.4 * _line_offsets[get_global_id(1)];
        
        float next_1 = 0.25 * _line_offsets[min((int)get_global_id(1) + 1, dim.y - 1)];
        float next_2 = 0.05 * _line_offsets[min((int)get_global_id(1) + 2, dim.y - 1)];
        
        rand_shift = (prev_2 + prev_1 + now_1 + next_1 + next_2);
    }
    
    
    //float rand_shift = _line_offsets[get_global_id(1)];
    
    float tape_shift_offset2 = tape_shift_offset + rand_shift;
    
    const float adj_x_red = (tape_shift_previous.r + tape_shift_offset2 + (float)coord.x + ((f_dim_2.x + tape_shift.r) * coord.y)) / (float)(f_dim_2.x*dim.y);
    
    const float adj_x_green = (tape_shift_previous.g + tape_shift_offset2 + (float)coord.x + ((f_dim_2.x + tape_shift.g) * coord.y)) / (float)(f_dim_2.x*dim.y);
    
    const float adj_x_blue = (tape_shift_previous.b + tape_shift_offset2 + (float)coord.x + ((f_dim_2.x + tape_shift.b) * coord.y)) / (float)(f_dim_2.x*dim.y);
    
    const float new_y = tape_align_master + 1.0f / 7.0f;
    const float adj_y_red = 1.5f * (new_y + tape_align.r);
    const float adj_y_green = 3.5f * (new_y + tape_align.g);
    const float adj_y_blue = 5.5f * (new_y + tape_align.b);
    
    const float2 f_dim = (float2)((float)dim.x, (float)dim.y);
    
    const float4 red = read_imagef(input, sampler, tape_coord_to_image_coord(adj_x_red, adj_y_red, f_dim));
    const float4 green = read_imagef(input, sampler, tape_coord_to_image_coord(adj_x_green, adj_y_green, f_dim));
    const float4 blue = read_imagef(input, sampler, tape_coord_to_image_coord(adj_x_blue, adj_y_blue, f_dim));
    
    write_imagef(output, coord, bt709yuvrgb((float4)(red.x, green.x, blue.x, 1.0f)));
}

#endif

