//
//  sand.cl
//  video-mush
//
//  Created by Josh McNamee on 13/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_sand_cl
#define video_mush_sand_cl


__kernel void sand_process(read_only image2d_t input, write_only image2d_t input_write, write_only image2d_t output, int wobble) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));

    const int2 dim = get_image_dim(input);
    
    const uint4 in = read_imageui(input, sampler, coord);
    
    uint4 out = (uint4)(0,0,0,0);
    
    if (in.x > 0 && in.y == 0.0) {
        const uint4 below = read_imageui(input, sampler, (int2)(coord.x+wobble, coord.y+1));
        if (below.x == 0) {
            out = in;
            write_imageui(input_write, coord, (uint4)(0,0,0,0));
            
            
            if (coord.x+wobble < dim.x && coord.x+wobble >= 0) {
                write_imageui(output, (int2)(coord.x+wobble, coord.y+1), out);
            }
        }
        
    }
    
    /*if (coord.y+1 == dim.y-1) {
        out = out + read_imageui(input, sampler, (int2)(coord.x, coord.y+1));
    }*/
    
}

__kernel void sand_bottom_bump(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const uint4 out = read_imageui(input, sampler, coord);
    write_imageui(output, (int2)(coord.x, coord.y), out);

}

__kernel void sand_copy_stills(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const uint4 out = read_imageui(input, sampler, coord);
    if (out.x > 0) {
        write_imageui(output, (int2)(coord.x, coord.y), out);
    }
}

__kernel void sand_add(__global uchar * input, read_only image2d_t output_read, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const uchar in = input[coord.x];
    if (in) {
		// 'randoms' is uint2 passed to kernel
		uint seed = 45636546 + coord.x;
		uint t = seed ^ (seed << 11);
		uint result = 34624654 ^ (34624654 >> 19) ^ (t ^ (t >> 8));

        const uint4 in_col = read_imageui(output_read, sampler, coord);
        write_imageui(output, coord, in_col+(uint4)(in, 0, result /(INT_MAX / 100), 0));
    }
}

__kernel void sand_to_image(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const uint4 in = read_imageui(input, sampler, coord);
    
    float4 out = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    if (in.x == 1) {
        if (in.y == 0) {
            out = (float4)(in.z/100.0f, in.z/100.0f, 0, 1.0f);
        } else if (in.y == 1) {
            out = (float4)(0.8f, 0.8f, 0.8f, 1.0f);
        }
    }
    
    write_imagef(output, coord, out);
}

__kernel void sand_clear(write_only image2d_t image) {
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    write_imageui(image, coord, (uint4)(0,0,0,0));
}

__kernel void sand_add_bumpers(write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    if ((coord.y+90) % 180 < 10) {
        if (coord.y > 530) {
            if (coord.x > 340 && coord.x < 940) {
                write_imageui(output, coord, (uint4)(1, 1, 0, 0));
            }
        } else if (coord.y > 350){
            if ((coord.x < 400 || coord.x > 880) || (coord.x > 540 && coord.x < 740)) {
                write_imageui(output, coord, (uint4)(1, 1, 0, 0));
            }
        } else if (coord.y > 170) {
            if ((coord.x < 300 || coord.x > 980) || (coord.x > 600 && coord.x < 680)) {
                write_imageui(output, coord, (uint4)(1, 1, 0, 0));
            }
        } else {
            if ((coord.x < 200 || coord.x > 1080) || (coord.x > 620 && coord.x < 660)) {
                write_imageui(output, coord, (uint4)(1, 1, 0, 0));
            }
        }
    }
}

#endif

