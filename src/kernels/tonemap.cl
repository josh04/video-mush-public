__kernel void tonemap(read_only image2d_t input, write_only image2d_t output, int hack_flag) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 size = get_image_dim(input);
	
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	
	float4 color;
	// gamma = 0.4545
	if (coord.x < size.x && coord.y < size.y) {
		color = read_imagef(input, sampler, coord);
//		tmo = color.x;
		
        if (hack_flag > 0) {
            color = max(pow(2.0f, color), 0.0f);
		}
            
//		tmo = tmo / hmean; //hmean scaling is ~~~
		
		color = color / (color + 1.0f);
        
		write_imagef(output, coord, color); // * 255 . We need to write out to 8 bit.
	}
}
