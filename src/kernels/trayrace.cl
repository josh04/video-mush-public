//
//  motion.cl
//  video-mush
//
//  Created by Josh McNamee on 12/08/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_motion_cl
#define video_mush_motion_cl

/*
__kernel void motion(write_only image2d_t output, read_only image2d_t previousFrame, read_only image2d_t motionVectors, read_only image2d_t currentFrame, read_only image2d_t previousDepth) {
    
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = (int2)get_image_dim(output);
    
    const float4 m = read_imagef(motionVectors, sampler_nearest, coord);
    
    const float2 coords_moved = (float2)(coord.x-m.x+0.5, coord.y-m.y+0.5);
    
    float4 previousD = (float4)(0.0, 0.0, 0.0, 0.0);
    if (coords_moved.x < dim.x && coords_moved.y < dim.y && coords_moved.x >= 0 && coords_moved.y >= 0) {
         previousD  = read_imagef(previousDepth, sampler_linear, coords_moved);
    }

    float4 out = read_imagef(currentFrame, sampler_nearest, coord);
    
    if (fabs(previousD.x - m.z) < 0.01) {
        out = read_imagef(previousFrame, sampler_linear, coords_moved);
    }
    
   // out = (float4)(fabs(previousD.x - m.z),fabs(previousD.x - m.z),fabs(previousD.x - m.z),1.0);
    
    write_imagef(output, coord, out);
}*/

__kernel void discontinuities(__global uchar * output, read_only image2d_t motionVectors, read_only image2d_t previousDepth, write_only image2d_t output_image) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(motionVectors);
    
    const float4 m = read_imagef(motionVectors, sampler_nearest, coord);
    
    const float2 coords_moved = (float2)(coord.x-m.x+0.5, coord.y-m.y+0.5);
    
    float4 previousD = (float4)(0.0, 0.0, 0.0, 0.0);
    
    if (coords_moved.x < dim.x && coords_moved.y < dim.y && coords_moved.x >= 0 && coords_moved.y >= 0) {
        previousD  = read_imagef(previousDepth, sampler_linear, coords_moved);
    }
    
    uchar out = 1;
    const float diff = fabs(previousD.x - m.z);
    if (diff < 1.0f) { // magic no. 3
        out = 0;
    } else {
        previousD = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    }// in par, nothing is moving
    
    //write_imagef(output_image, coord, (float4)(previousD.x, previousD.x, previousD.x, 1.0f));
    //write_imagef(output_image, coord, (float4)(m.z+diff, m.z, m.z, 1.0f));
	write_imagef(output_image, coord, (float4)(diff, diff, diff, 1.0f));
    
    output[dim.x*coord.y+coord.x] = out;
}

__kernel void fromCache(write_only image2d_t output, read_only image2d_t previousFrame, read_only image2d_t motionVectors, __global uchar * map) {
    
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    write_imagef(output, coord, (float4)(0.0, 0.0, 0.0, 0.0));
    
    const int2 dim = (int2)get_image_dim(output);
    
    if (map[coord.y*dim.x+coord.x] == 0) {
        const float4 m = read_imagef(motionVectors, sampler_nearest, coord);
        const float2 coords_moved = (float2)(coord.x-m.x+0.5, coord.y-m.y+0.5);
        if (coords_moved.x < dim.x && coords_moved.y < dim.y && coords_moved.x >= 0 && coords_moved.y >= 0) {
            const float4 out = read_imagef(previousFrame, sampler_linear, coords_moved);
            write_imagef(output, coord, out);
        }
    }
}

__constant uchar grid[] =  { 6,  5,  8,  12,
                            15,  1,  9, 13,
                             0, 10,  7,  4,
                            14,  3, 11,  2 }; // spatio-temporal upsampling on the gpu

//__constant uchar invgrid[] = { 8, 5, 15,  13,
                             // 11, 1, 0,  10,
                            //   2, 6, 9, 14,
                            //  3, 7, 12,  4  };// spatio-temporal upsampling on the gpu

__kernel void redraw(__global uchar * output, __global uchar * discontinuities, const uint width, const int offset, const int scale) {
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    uint out = discontinuities[width*coord.y+coord.x];
    
    //if ((coord.y % scale)*(scale)+(coord.x % scale) == (grid[offset] % (scale*scale))) {
        out = 1;
    //}
    
    output[width*coord.y+coord.x] = out;
}

__kernel void fromRay(write_only image2d_t output, read_only image2d_t redrawnFrame, __global uchar * map) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    
    const float4 out = read_imagef(redrawnFrame, sampler, coord);
    
    //if (out.x > 0) {
    //    write_imagef(output, coord, out);
    //}
    
    if (map[coord.y*dim.x+coord.x] == 1) {
        write_imagef(output, coord, out);
    }
    //if (out.x > 0 && map[coord.y*dim.x+coord.x] == 1) {
    //    write_imagef(output, coord, (float4)(1.0, 1.0, 0.0, 1.0));
    //}
}


__kernel void spatialClear(write_only image2d_t upSamples) {
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    write_imagef(upSamples, coord, (float4)(0.0f, 0.0f, 0.0f, 0.0f));
}

__kernel void scaleImage(write_only image2d_t output, read_only image2d_t input, __global uchar * map, int sc) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 dimI = get_image_dim(input);
//    const int2 dimO = get_image_dim(output);
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const int st = coord.y*sc*dimI.x + coord.x*sc;
    
    float4 in = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    float div = 0.0f;
    
    for (int i = 0; i < sc; ++i) {
        for (int j = 0; j < sc; ++j) {
            if (coord.x*sc+j < dimI.x && coord.y*sc+i < dimI.y && coord.x*sc+j >= 0 && coord.y*sc+i >= 0) {
                if (map[st+i*dimI.x+j] == 1) {
                    in += read_imagef(input, sampler_nearest, (int2)(coord.x*sc+j, coord.y*sc+i));
                    div += 1.0f;
                }
            }
        }
    }
    
    in = in / div;
    
    write_imagef(output, coord, in);
}

float spatialDot(const float x, const float o) {
    const float epsilon = 1e-5;
    
    return pow(max(epsilon, 1.0f - x/o), 3);
}

__kernel void spatialUpsample(write_only image2d_t upSamples, read_only image2d_t upSamples_read,
                              read_only image2d_t lowRes, read_only image2d_t geometry,
                              read_only image2d_t depth, __global uchar * map, char wmod, char hmod, 
	int offset, read_only image2d_t redrawnFrame, const int scale,
const float geomWeight, const float depthWeight, const float kWeight) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const int2 mod_coord = (int2)(coord.x + wmod, coord.y + hmod);
    
    const int2 dim = (int2)get_image_dim(upSamples);
    
    float4 upSample = read_imagef(upSamples_read, sampler_nearest, coord);
    float sample_weight = 0.0f;
                                  
    if (mod_coord.x < dim.x && mod_coord.y < dim.y && mod_coord.x >= 0 && mod_coord.y >= 0) {
        
        /*
        if ((coord.y % scale)*(scale)+(coord.x % scale) == (grid[offset] % (scale*scale))) {
            out = 1;
        }*/

        
        float xdiff2 =  (float)((grid[offset] % 4) % scale);
        float ydiff2 =  (float)(((grid[offset] - (grid[offset] % 4))/4 ) % scale);
        
        xdiff2 = -xdiff2 * (1.0f/scale);
        ydiff2 = -ydiff2 * (1.0f/scale);
        
        const float2 coord_norm = (float2)( ((float)mod_coord.x+0.5f)/dim.x, ((float)mod_coord.y+0.5f)/dim.y);
        //const float2 coord_norm = (float2)( ((float)mod_coord.x)/dim.x, ((float)mod_coord.y)/dim.y);
        
        // DISABLED BAD THING
        
        //const float2 coord_norm = (float2)( ((float)mod_coord.x+xdiff2)/dim.x, ((float)mod_coord.y+ydiff2)/dim.y);
        
        //const int2 coord_norm = (int2)((mod_coord.x - (mod_coord.x % 4))/4, (mod_coord.y - (mod_coord.y % 4))/4);
        
        const float4 mod_lowres = read_imagef(lowRes, sampler_linear, coord_norm);
        const float4 mod_highres = (float4)(0.0f, 0.0f, 0.0f, 1.0f);//read_imagef(redrawnFrame, sampler_nearest, mod_coord);
        const float4 mod_d = read_imagef(depth, sampler_nearest, mod_coord);
        const float4 mod_g = read_imagef(geometry, sampler_nearest, mod_coord);
        
        const float4 d = read_imagef(depth, sampler_nearest, coord);
        const float4 g = read_imagef(geometry, sampler_nearest, coord);
        
        const float geomDot = spatialDot(pow(max(0.0f, 1.0f - (g.x*mod_g.x + g.y*mod_g.y + g.z*mod_g.z)), 2.0f), geomWeight);
		const float depth_div = 1.0f;// 1024.0;
        const float depthDot = spatialDot(pow(d.x/depth_div - mod_d.x/depth_div, 2.0f), depthWeight); // was 1.0f JOSH
        
        //const float2 coordDiff = (float2)(coord.x-mod_coord.x, coord.y-mod_coord.y);
        /*
        float xdiff = fabs((float)(mod_coord.x % 4) - (float)(grid[offset] % 4));
        float ydiff = fabs( (float)(mod_coord.y % 4) - (float)(grid[offset] - (grid[offset] % 4) )/4.0f);
        const float kDot = spatialDot(sqrt(pow(xdiff, 2.0f) + pow(ydiff, 2.0f)), kWeight);
        */
        
        const float kDot = spatialDot(sqrt(pow((float)wmod, 2.0f) + pow((float)hmod, 2.0f)), kWeight);
        
        const float amp = 0.0f; //(float)map[mod_coord.y*dim.x+mod_coord.x]; CUT OUT THE HIGH RES
        
        /*const float*/ sample_weight = upSample.s3 + geomDot*depthDot*kDot;
        
        upSample = upSample*upSample.s3 + ((mod_lowres*(1.0f - amp) + mod_highres*amp)*geomDot*depthDot*kDot);
        
        //sample_weight += 0.1f;
		upSample.s3 = sample_weight;
        upSample.s0 = upSample.s0 / upSample.s3;
        upSample.s1 = upSample.s1 / upSample.s3;
        upSample.s2 = upSample.s2 / upSample.s3;
        //upSample = (mod_lowres);
        //upSample.s3 = 1.0f;
        /*float use =  depthDot*geomDot*kDot;
		upSample.s0 = use;
		upSample.s1 = use;
		upSample.s2 = use;
		upSample.s3 = 1.0f;*/
    }
    
    write_imagef(upSamples, coord, upSample);
}

__kernel void spatiotemporalUpsample(write_only image2d_t output,
                                     read_only image2d_t history, read_only image2d_t upSamples,
                                     __global uchar * map, read_only image2d_t motion) {
    
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    write_imagef(output, coord, (float4)(0.0, 0.0, 0.0, 0.0));
    
    const int2 dim = (int2)get_image_dim(output);
    
    //    if (map[coord.y*dim.x+coord.x] == 0) {
    
    const float4 m = read_imagef(motion, sampler_nearest, coord);
    
    float4 up_sample = read_imagef(upSamples, sampler_nearest, coord);
    
    const float sampleWeight = up_sample.s3;
    up_sample.s3 = 1.0f;
    
    float4 historyFrame = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
    float historyWeight = 0.0f;
    
    const float2 coords_moved = (float2)(coord.x-m.x+0.5, coord.y-m.y+0.5);
    if (coords_moved.x < (float)dim.x && coords_moved.y < (float)dim.y && coords_moved.x >= 0.0f && coords_moved.y >= 0.0f) {
        historyFrame = read_imagef(history, sampler_linear, coords_moved);
        historyWeight = historyFrame.s3;
        historyFrame.s3 = 1.0f;
    }
    //    }
    
    const float aaamp = map[coord.y*dim.x+coord.x];
    
    float full_weight = (1.0f - aaamp) * 0.9f * historyWeight;
	//historyFrame = (float4)(10.0f, 0.0f, 0.0f, 1.0f);
    float4 out = up_sample*sampleWeight + full_weight * historyFrame;
    out.s3 = sampleWeight + full_weight;
    out.s0 = out.s0 / out.s3;
    out.s1 = out.s1 / out.s3;
    out.s2 = out.s2 / out.s3;
    
    write_imagef(output, coord, out);//(float4)(historyWeight, historyWeight, historyWeight, 1.0f));
    
}

__kernel void historyBuffer(write_only image2d_t history, write_only image2d_t output, read_only image2d_t input) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const float4 in = read_imagef(input, sampler_nearest, coord);
    
    write_imagef(history, coord, in);
    write_imagef(output, coord, (float4)(in.x, in.y, in.z, 1.0f));
}

#endif

