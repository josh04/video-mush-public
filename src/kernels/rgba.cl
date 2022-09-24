// 0.2126, 0.7152, 0.0722, 0.0 BT.709
// 0.299, 0.587, 0.114 BT.601
__kernel void rgba(read_only image2d_t input, write_only image2d_t output) {
	const sampler_t samplr = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 size = get_image_dim(input);
	
//	const int2 coord = (int2)(size.x - get_global_id(0) - 1, size.y - get_global_id(1) - 1 );
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	
	float4 pixel = read_imagef(input, samplr, coord);

	if ( coord.x < size.x && coord.y < size.y ) {
		int indd = coord.x * 3 + (size.x * coord.y * 3);
		int jndd;

		if ((indd & 3) == 0) {
			jndd = (indd >> 2);
			int x = jndd % size.x;
			int y = (jndd - x)/size.x;
			float4 stolen_pixel = read_imagef(input, samplr, (int2)(x, y));
			pixel = (float4)(stolen_pixel.x, stolen_pixel.y, stolen_pixel.z, 1.0f);
		}

		if ((indd & 3) == 1) {

			jndd = ((indd-1) >> 2);
			int x = jndd % size.x;
			int y = (jndd - x)/size.x;

			float4 stolen_pixel = read_imagef(input, samplr, (int2)(x, y));

			pixel = (float4)(stolen_pixel.y, stolen_pixel.z, stolen_pixel.w, 1.0f);

		}

		if ((indd & 3) == 2) {

			jndd = ((indd+2) >> 2);
			int xndd = jndd % size.x;
			int yndd = (jndd - xndd)/size.x;

			float4 stolen_pixel = read_imagef(input, samplr, (int2)(xndd, yndd));

			if (xndd == 0) {
				xndd = size.x - 1;
				--yndd;
			} else {
				--xndd;
			}

			float4 stolen_pixel2 = read_imagef(input, samplr, (int2)(xndd, yndd));

			pixel = (float4)(stolen_pixel2.z, stolen_pixel2.w, stolen_pixel.x, 1.0f);
		}

		if ((indd & 3) == 3) {

			jndd = ((indd+1) >> 2);
			int x = jndd % size.x;
			int y = (jndd - x)/size.x;

			float4 stolen_pixel = read_imagef(input, samplr, (int2)(x, y));

			if (x == 0) {
				x = size.x - 1;
				--y;
			} else {
				--x;
			}

			float4 stolen_pixel2 = read_imagef(input, samplr, (int2)(x, y));

			pixel = (float4)(stolen_pixel2.w, stolen_pixel.x, stolen_pixel.y, 1.0f);
		}

		
	 }
     write_imagef(output, (int2)(coord.x, size.y - coord.y - 1), pixel);
}
