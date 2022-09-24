// 0.2126, 0.7152, 0.0722, 0.0 BT.709
// 0.299, 0.587, 0.114 BT.601
//__constant float4 weight4 = (float4)(0.299f, 0.587f, 0.114f, 0.0f); 

__kernel void luminance(read_only image2d_t input, write_only image2d_t luma, const float4 weight, 
					const float gamma, const float darken) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
	float4 pixel = max(read_imagef(input, sampler, coord), 0.0f);	 /// screw whoever put negative values in a pfm

    if (fabs(darken) > 1) {
        float min = pow(2.0f, darken);
        pixel = pixel * (float4)(min, min, min, 1.0f);
    }

    const float Y = min((float)log2(1e-6f + pixel.x * weight.x + pixel.y * weight.y + pixel.z * weight.z), MAXFLOAT);

    write_imagef(luma, coord, (float4)(Y, Y, Y, 1.0f));
}
