#ifndef MUSH_PAR_CL
#define MUSH_PAR_CL


// PAR

__kernel void parFinalise(__global float * finals, __global uint * samples, write_only image2d_t output) {
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	const int2 dim = get_image_dim(output);

	const int fpos = ((coord.y*dim.x + coord.x) * 3);
	const int spos = (coord.y*dim.x + coord.x);
	const float4 final = (float4)(finals[fpos], finals[fpos + 1], finals[fpos + 2], 1.0f);
	const uint sample = samples[spos];
	
	float4 out;
	
	out = final / (float)sample;
	
	if (sample == 0) {
		out = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
	}
	out = out + (float4)(0.001, 0.001, 0.001, 1.0f);

	write_imagef(output, coord, (float4)(out.x, out.y, out.z, 1.0f));
}

__kernel void parFlip(read_only image2d_t input, write_only image2d_t output) {
	const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

	const int2 dim = get_image_dim(output);

	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	const int2 coord_out = (int2)(coord.x, dim.y - coord.y - 1);

	const float4 in = read_imagef(input, sampler_nearest, coord);

	write_imagef(output, coord_out, in);
}


__kernel void parGLDepthR(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(output);
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 coord_out = (int2)(coord.x, dim.y - coord.y - 1);
    
    const float zNear = 0.1f;
    const float zFar = FLT_MAX;
    const float z_b = read_imagef(input, sampler_nearest, coord).x;
    
    float z_n = clamp(2.0f * (1.0f - z_b) - 1.0f, 0.0f, 1.0f);


    
    float in = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
	/*
	const float fac = 4.0f;

	float d_x = fac * (coord.x - (dim.x / 2.0f)) / (dim.x);
	float d_y = fac * ((float)dim.y/(float)dim.x) * (coord.y - (dim.y / 2.0f)) / (dim.y);
	float dist = sqrt(d_x*d_x + d_y*d_y);
	float dist2 = sqrt(in*in + dist*dist);
	*/
    write_imagef(output, coord_out, (float4)(in, in, in, 1.0f));
}

__kernel void parGLDepth(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 dim = get_image_dim(output);
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 coord_out = (int2)(coord.x, dim.y - coord.y - 1);
    
	const float zNear = 0.1f;
    const float zFar = FLT_MAX;
    //const float4 z_b = read_imagef(input, sampler_nearest, coord);
    const float z_b = read_imagef(input, sampler_nearest, coord).x;

    float z_n = clamp(2.0f * (1.0f - z_b), 0.0f, 1.0f);
    
	float in = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));
    
    write_imagef(output, coord_out, (float4)(in, in, in, 1.0f));
}

__kernel void parDepthToImage(__global float * depths, write_only image2d_t output, __global uint * samples) {
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	const int2 dim = get_image_dim(output);

	const int spos = (coord.y*dim.x + coord.x);
	const float sample = depths[spos]/(float)samples[spos];

	write_imagef(output, coord, (float4)(sample, sample, sample, 1.0f));
}

__kernel void parDepthToImageNoSamples(__global float * depths, write_only image2d_t output) {
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    const int spos = (coord.y*dim.x + coord.x);
    const float sample = depths[spos];
    
    write_imagef(output, coord, (float4)(sample, sample, sample, 1.0f));
}

__kernel void parVec3ToImage(read_only image2d_t input, write_only image2d_t output, __global uint * samples) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    const int spos = (coord.y*dim.x + coord.x);
    const uint sample = samples[spos];
    
    const float4 in = read_imagef(input, sampler_nearest, coord) / (float)sample;
    
    write_imagef(output, coord, (float4)(in.x, in.y, in.z, 1.0f));
}


__kernel void amd_divide(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    float4 in = read_imagef(input, sampler_nearest, coord);
    in = in/in.w;
    write_imagef(output, coord, (float4)(in.x, in.y, in.z, 1.0f));
}

#endif
