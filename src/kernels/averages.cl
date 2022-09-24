__kernel void averages(read_only image2d_t input, write_only image2d_t output, int width, int height, unsigned int scale) {
	const sampler_t samplr = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	
	int2 _scale = (int2)(scale, scale);
	
	int2 start = coord * _scale;
	int2 end = (coord + 1) * _scale;
	
	float avg = 0.0f;
	float4 lum;
	int count = 0;
	float hmean = 0.0f;
	float lmin = MAXFLOAT;
	float lmax = 0.0f;
	
	for (int y = start.y; y < end.y; ++y) {
		for (int x = start.x; x < end.x; ++x) {
			if (x <= width && y <= height) {
				lum = read_imagef(input, samplr, (int2)(x, y));
				hmean += lum.s0;
				lmax = (lmax < lum.s1) ? lum.s1 : lmax;
				lmin = (lmin > lum.s2) ? lum.s2 : lmin;
				avg += lum.s3;
				++count;
			}
		}
	}

	if (count == 0) {
		count = 1;
	}
	
	write_imagef(output, coord, (float4)((hmean / (float)count), lmax, lmin, avg)); // hmean
}

__kernel void toGray(read_only image2d_t input, write_only image2d_t output) {
	const sampler_t samplr = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	float4 lum = read_imagef(input, samplr, (int2)(coord.x, coord.y));
	float outlum = 0.33333*lum.s0 + 0.33333*lum.s1 + 0.33333+lum.s2;
	write_imagef(output, coord, (float4)(outlum, outlum, outlum, 1.0)); // hmean
}

