//
//  laplace.cl
//  video-mush
//
//  Created by Josh McNamee on 04/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_laplace_cl
#define video_mush_laplace_cl

__kernel void laplace(read_only image2d_t input, write_only image2d_t output) {
    const float4 weight4 = (float4)(0.299f, 0.587f, 0.114f, 0.0f);
    const float weights[9] = {  -1.0f, -1.0f, -1.0f,
                                -1.0f, 8.0f, -1.0f,
                                -1.0f, -1.0f, -1.0f};
    /*const float weights[9] = {  -0.0f, -1.0f, -0.0f,
                                -1.0f, 4.0f, -1.0f,
                                -0.0f, -1.0f, -0.0f};*/
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 size = get_image_dim(input);
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	
    float sum = 0.0f;
		
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            const int w = coord.x - 1 + i;
            const int v = coord.y - 1 + j;
            if ( w > 0 && v > 0 && w < size.x && v < size.y) {
                const float4 in4 = read_imagef(input, sampler, (int2)(coord.x-1+i, coord.y-1+j));
                
                const float in = dot(in4, weight4);
                
                sum += weights[i*3 + j] * in;
            }
        }
    }
    
    if (sum > 0.0f) {
        write_imagef(output, coord, (float4)(fabs(sum), 0.0f, 0.0f, 1.0f));
    } else {
        write_imagef(output, coord, (float4)(0.0f, 0.0f, fabs(sum), 1.0f));
    }
}

__kernel void edge_clear(write_only image2d_t edges) {
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    write_imagef(edges, coord, (float4)(0.0f, 0.0f, 0.0f, 0.0f));
}

__kernel void edge_threshold(read_only image2d_t input, read_only image2d_t laplace, write_only image2d_t output, const float threshold) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const float4 la = read_imagef(laplace, sampler, coord);
    const float lap = la.x+la.y+la.z;
    
    if (lap > threshold) {
        float4 pixel = read_imagef(input, sampler, coord);
        pixel.s1 = 1.0f;
        write_imagef(output, coord, pixel);
    }
}

__kernel void edge_samples(read_only image2d_t input, write_only image2d_t output) {
    const int2 coord = (int2)(get_global_id(0) * 32, get_global_id(1) * 32);
    const int2 coord_s = (int2)(get_global_id(0), get_global_id(1));
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const float4 in = read_imagef(input, sampler, coord);
    write_imagef(output, coord_s, in);
}

float4 diffuse_diag_avg(read_only image2d_t input, const int2 coord, const int2 dim) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 upleft = (int2)(coord.x-1, coord.y-1);
    const int2 upright = (int2)(coord.x+1, coord.y-1);
    const int2 downleft = (int2)(coord.x-1, coord.y+1);
    const int2 downright = (int2)(coord.x+1, coord.y+1);
    
    float4 upleftf = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    float4 uprightf = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    float4 downleftf = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    float4 downrightf = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    
    if (upright.x < dim.x) {
        if (downright.y < dim.y) {
            downrightf = read_imagef(input, sampler, downright);
        }
        if (upright.y > 0) {
            uprightf = read_imagef(input, sampler, upright);
        }
    }
    if (downleft.x > 0) {
        if (downleft.y < dim.y) {
            downleftf = read_imagef(input, sampler, downleft);
        }
        if (upleft.y > 0) {
            upleftf = read_imagef(input, sampler, upleft);
        }
    }
    
    float4 out = (float4)(0.0f, 1.0f, 0.0f, 0.0f);
    unsigned int o = 0;
    
    if (upleftf.s1 > 0.5f) {
        out += upleftf;
        ++o;
    }
    
    if (uprightf.s1 > 0.5f) {
        out += uprightf;
        ++o;
    }
    
    if (downleftf.s1 > 0.5f) {
        out += downleftf;
        ++o;
    }
    
    if (downrightf.s1 > 0.5f) {
        out += downrightf;
        ++o;
    }
    
    if (o > 0) {
        out = out/(float)o;
    } else {
        out = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    }
    return out;
}

float4 diffuse_horiz_avg(read_only image2d_t input, const int2 coord, const int2 dim) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 left = (int2)(coord.x-1, coord.y);
    const int2 right = (int2)(coord.x+1, coord.y);
    const int2 up = (int2)(coord.x, coord.y-1);
    const int2 down = (int2)(coord.x, coord.y+1);
    
    float4 leftf = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    float4 rightf = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    float4 upf = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    float4 downf = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    
    
        if (down.y < dim.y) {
            downf = read_imagef(input, sampler, down);
        }
        if (up.y > 0) {
            upf = read_imagef(input, sampler, up);
        }
        
        if (right.x < dim.x) {
            rightf = read_imagef(input, sampler, right);
        }
        if (left.x > 0) {
            leftf = read_imagef(input, sampler, left);
        }
    
    float4 out = (float4)(0.0f, 1.0f, 0.0f, 0.0f);
    unsigned int o = 0;
    
    if (leftf.s1 > 0.5f) {
        out += leftf;
        ++o;
    }
    
    if (rightf.s1 > 0.5f) {
        out += rightf;
        ++o;
    }
    
    if (downf.s1 > 0.5f) {
        out += downf;
        ++o;
    }
    
    if (upf.s1 > 0.5f) {
        out += upf;
        ++o;
    }
    
    if (o > 0) {
        out = out/(float)o;
    } else {
        out = (float4)(1.0f, 0.0f, 0.0f, 0.0f);
    }
    return out;
}

__kernel void diffuse_push(read_only image2d_t input, write_only image2d_t output) {
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 upcoord = coord * 2;
    const int2 dim = get_image_dim(input);
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    if (upcoord.x < dim.x && upcoord.y < dim.y) {
        const float4 in = read_imagef(input, sampler, upcoord);
        if (in.s1 < 0.5f) {
            const float4 out = diffuse_diag_avg(input, upcoord, dim);
            write_imagef(output, coord, out);
        } else {
            write_imagef(output, coord, in);
        }
    }
}

__kernel void diffuse_pull_copy(read_only image2d_t input, read_only image2d_t read_output, write_only image2d_t output) {
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 upcoord = coord * 2;
    const int2 dim = get_image_dim(output);
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const float4 test = read_imagef(read_output, sampler, upcoord);
    if (test.s1 < 0.5f && upcoord.x < dim.x && upcoord.y < dim.y) {
        const float4 pull = read_imagef(input, sampler, coord);
        write_imagef(output, upcoord, pull);
    }
}

__kernel void diffuse_pull_diag(read_only image2d_t read_output, write_only image2d_t output) {
    const int2 coord = (int2)(get_global_id(0)*2+1, get_global_id(1)*2+1);
    const int2 dim = get_image_dim(read_output);
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const float4 test = read_imagef(read_output, sampler, coord);
    if (test.s1 < 0.5f) {
        const float4 avg = diffuse_diag_avg(read_output, coord, dim);
        if (coord.x < dim.x && coord.y < dim.y) {
            write_imagef(output, coord, avg);
        }
    }
}


__kernel void diffuse_pull_horiz(read_only image2d_t read_output, write_only image2d_t output) {
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const int2 dim = get_image_dim(output);
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const float4 test = read_imagef(read_output, sampler, coord);
    if (test.s1 < 0.5f) {
        const float4 avg = diffuse_horiz_avg(read_output, coord, dim);
        write_imagef(output, coord, avg);
    }
}


#endif

