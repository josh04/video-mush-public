/*__constant float4 uyvy2rgb[] = {
	(float4)(0.0f, 1.164f, 1.793f, 0.0f),
	(float4)(-0.213f, 1.164f, -0.533f, 0.0f),
	(float4)(2.112f, 1.164f, 0.0f, 0.0f),
	(float4)(0.0f, 0.0f, 1.793f, 1.164f),
	(float4)(-0.213f, 0.0f, -0.533f, 1.164f),
	(float4)(2.112f, 0.0f, 0.0f, 1.164f),
	(float4)(-128.0f, -16.0f, -128.0f, -16.0f)
};
    
    
 
    float4 bt709yuvrgb(float4 input) {
        input.y = input.y - 0.5f;
        input.z = input.z - 0.5f;
        const float r = input.x*1 + input.y*0 +         input.z*1.570;
        const float g = input.x*1 + input.y*-0.187 +    input.z*-0.467;
        const float b = input.x*1 + input.y*1.856 +     input.z*0;
        return (float4)(r, g, b, 1.0f);
    }
/*	(float4)(0.0f,		1.0f,	1.280f,		0.0f),
	(float4)(-0.215f,	1.0f,	-0.381f,	0.0f),
	(float4)(2.128f,	1.0f,	0.0f,		0.0f),

	(float4)(0.0f,		0.0f,	1.280f,		1.0f),
	(float4)(-0.215f,	0.0f,	-0.381f,	1.0f),
	(float4)(2.128f,	0.0f,	0.0f,		1.0f),

	(float4)(-128.0f, -16.0f, -128.0f, -16.0f)*/

__constant float4 uyvy2rgb[] = {
    (float4)(0.0f, 1.0f, 1.570f, 0.0f),
    (float4)(-0.187f, 1.0f, -0.467f, 0.0f),
    (float4)(1.856f, 1.0f, 0.0f, 0.0f),
    (float4)(0.0f, 0.0f, 1.570f, 1.0f),
    (float4)(-0.187f, 0.0f, -0.467f, 1.0f),
    (float4)(1.856f, 0.0f, 0.0f, 1.0f),
    (float4)(-128.0f, -16.0f, -128.0f, -16.0f)
};

float4 promote(uint4 x) {
	return (float4)((float)x.s0, (float)x.s1, (float)x.s2, (float)x.s3);
}

float4 promoteShort(ushort4 x) {
	return (float4)((float)x.s0, (float)x.s1, (float)x.s2, (float)x.s3);
}

__kernel void convert(write_only image2d_t rgbaFrame, read_only image2d_t yuvFrame, int invert) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);
	int width = get_global_size(0), height = get_global_size(1);
	int hOffset = get_global_offset(0), vOffset = get_global_offset(1);

	float4 in = (promote(read_imageui(yuvFrame, sampler, (int2)(x, y))) + uyvy2rgb[6]) / 255.0f;

	float4 out0 = clamp((float4)(dot(in, uyvy2rgb[0]), dot(in, uyvy2rgb[1]), dot(in, uyvy2rgb[2]), 1.0f), 0.0f, 1.0f);
	float4 out1 = clamp((float4)(dot(in, uyvy2rgb[3]), dot(in, uyvy2rgb[4]), dot(in, uyvy2rgb[5]), 1.0f), 0.0f, 1.0f);

	if (invert == 0) {
		write_imagef(rgbaFrame, (int2)((x - hOffset) * 2, y - vOffset), out0);
		write_imagef(rgbaFrame, (int2)((x - hOffset) * 2 + 1, y - vOffset), out1);
	} else {
		write_imagef(rgbaFrame, (int2)((width - x + hOffset - 1) * 2 + 1, height - y + vOffset - 1), out0);
		write_imagef(rgbaFrame, (int2)((width - x + hOffset - 1) * 2, height - y + vOffset - 1), out1);
	}
}

__kernel void convert10(write_only image2d_t rgbaFrame, __global int * yuvFrame) {
	int x = get_global_id(0), y = get_global_id(1);
	int width = get_global_size(0), height = get_global_size(1);

	int start = x*4 + width*4*y;

	int alpha = yuvFrame[start];
	int beta = yuvFrame[start+1];
	int gamma = yuvFrame[start+2];
	int delta = yuvFrame[start+3];

	ushort cr0 = 0;
	ushort y0 = 0;
	ushort cb0 = 0;
	
	ushort y2 = 0;
	ushort cb2 = 0;
	ushort y1 = 0;
	
	ushort cb4 = 0;
	ushort y3 = 0;
	ushort cr2 = 0;
	
	ushort y5 = 0;
	ushort cr4 = 0;
	ushort y4 = 0;
	
	cb0 = (alpha) & 0x3FF;
	y0 = (alpha >> 10) & 0x3FF;
	cr0 = (alpha >> 20) & 0x3FF;
	
	y1 = (beta) & 0x3FF;
	cb2 = (beta >> 10) & 0x3FF;
	y2 = (beta >> 20) & 0x3FF;
	
	cr2 = (gamma) & 0x3FF;
	y3 = (gamma >> 10) & 0x3FF;
	cb4 = (gamma >> 20) & 0x3FF;
	
	y4 = (delta) & 0x3FF;
	cr4 = (delta >> 10) & 0x3FF;
	y5 = (delta >> 20) & 0x3FF;

//	float4 in = (promote(read_imageui(yuvFrame, sampler, (int2)(x, y))) + uyvy2rgb[6]) / 255.0f;
/*
	float4 v210 = (float4)(-512.0f, -64.0f, -512.0f, -64.0f);
	
	float4 al = (promoteShort((ushort4)(cb0, y0, cr0, y1)) + v210) / 1023.0f;
	float4 be = (promoteShort((ushort4)(cb2, y2, cr2, y3)) + v210) / 1023.0f;
	float4 ga = (promoteShort((ushort4)(cb4, y4, cr4, y5)) + v210) / 1023.0f;

	float4 out0 = clamp((float4)(dot(al, uyvy2rgb[0]), dot(al, uyvy2rgb[1]), dot(al, uyvy2rgb[2]), 1.0f), 0.0f, 1.0f);
	float4 out1 = clamp((float4)(dot(al, uyvy2rgb[3]), dot(al, uyvy2rgb[4]), dot(al, uyvy2rgb[5]), 1.0f), 0.0f, 1.0f);
	float4 out2 = clamp((float4)(dot(be, uyvy2rgb[0]), dot(be, uyvy2rgb[1]), dot(be, uyvy2rgb[2]), 1.0f), 0.0f, 1.0f);
	float4 out3 = clamp((float4)(dot(be, uyvy2rgb[3]), dot(be, uyvy2rgb[4]), dot(be, uyvy2rgb[5]), 1.0f), 0.0f, 1.0f);
	float4 out4 = clamp((float4)(dot(ga, uyvy2rgb[0]), dot(ga, uyvy2rgb[1]), dot(ga, uyvy2rgb[2]), 1.0f), 0.0f, 1.0f);
	float4 out5 = clamp((float4)(dot(ga, uyvy2rgb[3]), dot(ga, uyvy2rgb[4]), dot(ga, uyvy2rgb[5]), 1.0f), 0.0f, 1.0f);
*/
	
	
    float4 v210 = (float4)(-512.0f, -64.0f, -512.0f, -64.0f);
    float twoFootRoom = 1023.0f - 64.0f - 64.0f;
    float footAndHead = 1023.0f - 64.0f - 84.0f;
    char useExtended = 0;
    if (useExtended == 1) {
        v210 = (float4)(-512.0f, 4.0f, -512.0f, 4.0f);
        twoFootRoom = 1015.0f;
        footAndHead = 1015.0f;

    }
    
	float4 al = (promoteShort((ushort4)(cb0, y0, cr0, y1)) + v210) / (float4)(twoFootRoom, footAndHead, twoFootRoom, footAndHead);
	float4 be = (promoteShort((ushort4)(cb2, y2, cr2, y3)) + v210) / (float4)(twoFootRoom, footAndHead, twoFootRoom, footAndHead);
	float4 ga = (promoteShort((ushort4)(cb4, y4, cr4, y5)) + v210) / (float4)(twoFootRoom, footAndHead, twoFootRoom, footAndHead);
	
	float4 out0 = clamp((float4)(dot(al, uyvy2rgb[0]), dot(al, uyvy2rgb[1]), dot(al, uyvy2rgb[2]), 1.0f), 0.0f, 1.0f);
	float4 out1 = clamp((float4)(dot(al, uyvy2rgb[3]), dot(al, uyvy2rgb[4]), dot(al, uyvy2rgb[5]), 1.0f), 0.0f, 1.0f);
	float4 out2 = clamp((float4)(dot(be, uyvy2rgb[0]), dot(be, uyvy2rgb[1]), dot(be, uyvy2rgb[2]), 1.0f), 0.0f, 1.0f);
	float4 out3 = clamp((float4)(dot(be, uyvy2rgb[3]), dot(be, uyvy2rgb[4]), dot(be, uyvy2rgb[5]), 1.0f), 0.0f, 1.0f);
	float4 out4 = clamp((float4)(dot(ga, uyvy2rgb[0]), dot(ga, uyvy2rgb[1]), dot(ga, uyvy2rgb[2]), 1.0f), 0.0f, 1.0f);
	float4 out5 = clamp((float4)(dot(ga, uyvy2rgb[3]), dot(ga, uyvy2rgb[4]), dot(ga, uyvy2rgb[5]), 1.0f), 0.0f, 1.0f);
	
	int end = x*6;
	int image_width = get_image_width(rgbaFrame);
	
	if (end < image_width) {
		write_imagef(rgbaFrame, (int2)(end, y), out0);
	}
	if (end+1 < image_width) {
		write_imagef(rgbaFrame, (int2)(end+1, y), out1);
	}
	if (end+2 < image_width) {
		write_imagef(rgbaFrame, (int2)(end+2, y), out2);
	}
	if (end+3 < image_width) {
		write_imagef(rgbaFrame, (int2)(end+3, y), out3);
	}
	if (end+4 < image_width) {
		write_imagef(rgbaFrame, (int2)(end+4, y), out4);
	}
	if (end+5 < image_width) {
		write_imagef(rgbaFrame, (int2)(end+5, y), out5);
	}
}

uchar createMask(uchar a, uchar b)
{
   uchar r = 0;
   for (uchar i=a; i<=b; i++)
       r |= 1 << i;

   return r;
}

__kernel void average(write_only image2d_t output, read_only image2d_t input, unsigned int width, unsigned int height, unsigned int scale) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);

	float4 sum = 0.0f;
	int count = 0;

	for (int j = y * scale; j < (y + 1) * scale; ++j) {
		for (int i = x * scale; i < (x + 1) * scale; ++i) {
			if (i < width && j < height) {
				float4 p = read_imagef(input, sampler, (int2)(i, j));
				sum += p;
				++count;
			}
		}
	}

	write_imagef(output, (int2)(x, y), sum / (float)count);
}

__kernel void difference(write_only image2d_t output, read_only image2d_t alpha, read_only image2d_t beta) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);

	write_imagef(output, (int2)(x, y), fabs(read_imagef(alpha, sampler, (int2)(x, y)) - read_imagef(beta, sampler, (int2)(x, y))));
}

__kernel void sum(write_only image2d_t output, read_only image2d_t input, unsigned int width, unsigned int height, unsigned int scale) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);

	float4 sum = 0.0f;

	for (int j = y; j < y + scale; ++j) {
		for (int i = x; i < x + scale; ++i) {
			if (i < width && j < height) {
				sum += read_imagef(input, sampler, (int2)(i, j));
			}
		}
	}

	write_imagef(output, (int2)(x, y), sum);
}

