__kernel void floatToHalf(read_only image2d_t in, __global half * out) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE;
	int2 pos = (int2)(get_global_id(0), get_global_id(1));
	int2 size = (int2)(get_global_size(0), get_global_size(1));
	size_t off = pos.y * size.x + pos.x;
	
	float4 p = read_imagef(in, sampler, pos);

	vstore_half4(p, off, out);
}

