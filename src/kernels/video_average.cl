#ifndef VIDEO_AVERAGE_CL
#define VIDEO_AVERAGE_CL

__kernel void video_average(read_only image2d_t input, read_only image2d_t out_read, write_only image2d_t output, int count, int deduct) {
	const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));

	const int2 dim = get_image_dim(input);

	const float4 i = pow(read_imagef(input, sampler_nearest, coord), (float4)(2.2f, 2.2f, 2.2f, 1.0f));
	const float4 prev = pow(read_imagef(out_read, sampler_nearest, coord), (float4)(2.2f, 2.2f, 2.2f, 1.0f));

	const float4 p = prev - deduct * prev / count;
	const float4 o = (i + p * (count - 1)) / count;

	write_imagef(output, coord, pow(o, (float4)(0.4545f, 0.4545f, 0.4545f, 1.0f)));
}

#endif
