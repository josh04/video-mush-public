
float rec709backward(float v) {
    const float a = 0.099;
    
    float x;
    if (v < 0.081) {
        x = v / 4.5;
    } else {
        x = pow((v + a) / (1.0f + a), 1.0f / 0.45f);
    }
    return x;
}

float rec709forward(float l) {
    float v;
    
    if (l < 0.018) {
        v = 4.5 * l;
    } else {
        v = (1.099 * pow(l, 0.45f)) - 0.099;
    }
    
    return v;
}

float4 rec709forward4(float4 in_a) {
	return (float4)(rec709forward(in_a.x), rec709forward(in_a.y), rec709forward(in_a.z), 1.0f);
}

float4 rec709backward4(float4 in_a) {
	return (float4)(rec709backward(in_a.x), rec709backward(in_a.y), rec709backward(in_a.z), 1.0f);
}


__kernel void encodeRec709(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 in_a = read_imagef(input, sampler, (int2)(x, y));

	float4 temp = rec709forward4(in_a);
    
    write_imagef(output, (int2)(x, y), temp);
}

__kernel void decodeRec709(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 in_a = read_imagef(input, sampler, (int2)(x, y));
    
	float4 temp = rec709backward4(in_a);
    
    write_imagef(output, (int2)(x, y), temp);
}
