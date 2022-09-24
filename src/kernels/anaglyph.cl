#ifndef ANAGLYPH_CL
#define ANAGLYPH_CL


__kernel void anaglyph(read_only image2d_t left, read_only image2d_t right, write_only image2d_t output, uchar mode) {

    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int2 coord = {get_global_id(0), get_global_id(1)};

    const float4 l = read_imagef(left, sampler, coord);
    const float4 r = read_imagef(right, sampler, coord);

    float4 out;
    switch (mode) {
	case 0:
		out = (float4)(l.x, r.y, 0.0f, 1.0f);
		break;
	default:
	case 1:
		out = (float4)(l.x, r.y, r.z, 1.0f);
		break;
    }

    write_imagef(output, coord, out);
}

#endif
