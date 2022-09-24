// 0.2126, 0.7152, 0.0722, 0.0 BT.709
// 0.299, 0.587, 0.114 BT.601
__kernel void planarRGBtoRGBA(__global ushort * input, write_only image2d_t output, const uint width, const uint height) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	
	float4 pixel;

	pixel.x = (float)(input[coord.x+coord.y*width] / 65535.0f);
	pixel.y = (float)(input[coord.x+coord.y*width + width*height] / 65535.0f);
	pixel.z = (float)(input[coord.x+coord.y*width + width*height*2] / 65535.0f);
	pixel.w = 1.0f;

    write_imagef(output, (int2)(coord.x, coord.y), pixel);
}


float4 promoteShort2(ushort4 x) {
	return (float4)((float)x.s0, (float)x.s1, (float)x.s2, (float)x.s3);
}

__constant float4 uyvy2rgb2[] = {
	(float4)(0.0f, 1.164f, 1.793f, 0.0f),
	(float4)(-0.213f, 1.164f, -0.533f, 0.0f),
	(float4)(2.112f, 1.164f, 0.0f, 0.0f),
	(float4)(0.0f, 0.0f, 1.793f, 1.164f),
	(float4)(-0.213f, 0.0f, -0.533f, 1.164f),
	(float4)(2.112f, 0.0f, 0.0f, 1.164f),
	(float4)(-128.0f, -16.0f, -128.0f, -16.0f)
    
    /*	(float4)(0.0f,		1.0f,	1.280f,		0.0f),
     (float4)(-0.215f,	1.0f,	-0.381f,	0.0f),
     (float4)(2.128f,	1.0f,	0.0f,		0.0f),
     
     (float4)(0.0f,		0.0f,	1.280f,		1.0f),
     (float4)(-0.215f,	0.0f,	-0.381f,	1.0f),
     (float4)(2.128f,	0.0f,	0.0f,		1.0f),
     
     (float4)(-128.0f, -16.0f, -128.0f, -16.0f)*/
};

__kernel void planarYUVtoRGBA(__global ushort * input, write_only image2d_t output, const uint width, const uint height) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	
	float4 pixel;
    
	pixel.x = (float)(input[coord.x*2+coord.y*width]);
	pixel.w = (float)(input[coord.x*2+1+coord.y*width]);
	pixel.y = (float)(input[coord.x+coord.y*width/2 + width*height]);
	pixel.z = (float)(input[coord.x+coord.y*width/2 + (int)(width*height*1.5)]);
    
	float4 v210 = (float4)(-512.0f, -64.0f, -512.0f, -64.0f);
	
	float4 al = (promoteShort2((ushort4)(pixel.y, pixel.x, pixel.z, pixel.w)) + v210) / 255.0f;
	float4 out0 = clamp((float4)(dot(al, uyvy2rgb2[0]), dot(al, uyvy2rgb2[1]), dot(al, uyvy2rgb2[2]), 1.0f), 0.0f, 4.0f);
    float4 out1 = clamp((float4)(dot(al, uyvy2rgb2[3]), dot(al, uyvy2rgb2[4]), dot(al, uyvy2rgb2[5]), 1.0f), 0.0f, 4.0f);
    
//    out0 = (float4)(al.y, al.y, al.y, 1.0f);
//    out1 = (float4)(be.y, be.y, be.y, 1.0f);
    
    write_imagef(output, (int2)(coord.x*2, coord.y), pow(out0, 2.2f));
    write_imagef(output, (int2)(coord.x*2+1, coord.y), pow(out1, 2.2f));
}
