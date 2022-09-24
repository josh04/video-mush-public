/**
 * References:
 * https://code.google.com/p/libcl/
 * http://people.csail.mit.edu/sparis/bf_course/
 * http://www.cse.iitd.ernet.in/~pkalra/csl783/bilateral-filtering-ICCV98.pdf
 * 
 * \input	the input image, any float4 format so long as there is an appropriate conversion matrix
 * \output	the output image, will be in same format as input
 * \sigmaD	Spatial sigma
 * \sigmaR	Range sigma
 * \window	Half window size to use
 * \convert	color converstion matrix
 */


/*
 
 bilateral->setArg(0, *lumaImage); // Input
 bilateral->setArg(1, *filterImage); // Output
 bilateral->setArg(2, 1.0f / (2.0f * 4.0f*4.0f)); // sigma s, range 0.1 - 16.0 (default, 0.4f). 1.0f / (2.0f * sigmaS*sigmaS)
 bilateral->setArg(3, 1.0f / (2.0f * 2.0f*2.0f)); // sigma r, range 0.1 - 1.0 - inf (default, 0.2f). 1.0f / (2.0f * sigmaR*sigmaR)
 bilateral->setArg(4, 5); // Half window, range 0 - inf (default, 1-8)
 */
__kernel void bilateral(read_only image2d_t input, write_only image2d_t output, float sigmaS, float sigmaR, int halfWindow) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 size = get_image_dim(input); // Image size
	
	int2 q, p = (int2)(get_global_id(0), get_global_id(1));
	
	if (p.x <= size.x && p.y <= size.y) {
		const float ip = read_imagef(input, sampler, p).x;
		
		float sum = 0.0f, norm = 0.0f;
		
		const int2 start = (int2)(p.x - halfWindow, p.y - halfWindow), end = (int2)(p.x + halfWindow, p.y + halfWindow);
		
		for (q.y = start.y; q.y <= end.y; ++q.y) {
			for (q.x = start.x; q.x <= end.x; ++q.x) {
				if (q.x >= 0 && q.y >= 0 && q.x <= size.x && q.y <= size.y) {
					float iq = read_imagef(input, sampler, q).x;
					
					float2 space = (float2)(p.x - q.x, p.y - q.y);
					float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));
					
					float range = ip - iq;
					float gr = exp(-sigmaR * (range*range));
					
					float factor = gs * gr;
					
					sum += factor * iq;
					norm += factor;
				}
			}
		}
		
		const float out = sum / norm;
		write_imagef(output, p, (float4)(out, out, out, out));
	}
}

__kernel void bilateralOpt(read_only image2d_t input, write_only image2d_t output, float sigmaS, float sigmaR, int halfWindow, __local float * buffer) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 size = get_image_dim(input); // Image size
	
	int2 q, p = (int2)(get_global_id(0), get_global_id(1));
	int2 lId = (int2)(get_local_id(0), get_local_id(1));
	int2 lDim = (int2)(get_local_size(0), get_local_size(1));

	const float ip = read_imagef(input, sampler, p).x;
	buffer[lId.y * lDim.x + lId.x] = ip;
	barrier(CLK_LOCAL_MEM_FENCE);
	
	if (p.x <= size.x && p.y <= size.y) {	
		float sum = 0.0f, norm = 0.0f;
		
		const int2 start = (int2)(p.x - halfWindow, p.y - halfWindow), end = (int2)(p.x + halfWindow, p.y + halfWindow);
		
		for (q.y = start.y; q.y <= end.y; ++q.y) {
			for (q.x = start.x; q.x <= end.x; ++q.x) {
				if (q.x >= 0 && q.y >= 0 && q.x <= size.x && q.y <= size.y) {
					int2 lOffset = lId + p - q;

					float iq;
					if (lOffset.x >= 0 && lOffset.x < lDim.x && lOffset.y >= 0 && lOffset.y < lDim.y) {
						iq = buffer[lOffset.y * lDim.x + lOffset.x];
					} else {
						iq = read_imagef(input, sampler, q).x;
					}
					
					float2 space = (float2)(p.x - q.x, p.y - q.y);
					float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));
					
					float range = ip - iq;
					float gr = exp(-sigmaR * (range*range));
					
					float factor = gs * gr;
					
					sum += factor * iq;
					norm += factor;
				}
			}
		}
		
		const float out = sum / norm;
		write_imagef(output, p, (float4)(out, out, out, out));
	}
}

__kernel void bilinear(read_only image2d_t input, write_only image2d_t output, const float sigmaS, const float sigmaR, const int halfWindow) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	const int2 size = get_image_dim(input); // Image size
	int2 q, p = (int2)(get_global_id(0), get_global_id(1));
	if (p.x <= size.x && p.y <= size.y) {
		float ip = read_imagef(input, sampler, p).x;
		float sum = 0.0f, norm = 0.0f;
		sum += ip;
		norm += 1.0f;
		q = p;
		for (q.x = p.x - halfWindow; q.x <= p.x + halfWindow; ++q.x) {
			if (q.x >= 0 && q.x <= size.x && q.x != p.x) {
				float iq = read_imagef(input, sampler, q).x;
				float2 space = (float2)(p.x - q.x, p.y - q.y);
				float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));
				float range = ip - iq;
				float gr = exp(-sigmaR * (range*range));
				float factor = gs * gr;
				sum += factor * iq;
				norm += factor;
			}
		}
		q = p;
		for (q.y = p.y - halfWindow; q.y <= p.y + halfWindow; ++q.y) {
			if (q.y >= 0 && q.y <= size.y && q.y != p.y) {
				float iq = read_imagef(input, sampler, q).x;
				float2 space = (float2)(p.x - q.x, p.y - q.y);
				float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));
				float range = ip - iq;
				float gr = exp(-sigmaR * (range*range));
				float factor = gs * gr;
				sum += factor * iq;
				norm += factor;
			}
		}
		for (q.x = p.x - halfWindow, q.y = p.y - halfWindow; q.x <= p.x + halfWindow; ++q.x, ++q.y) {
			if (q.x >= 0 && q.y >= 0 && q.x <= size.x && q.y <= size.y && q.x != p.x) {
				float iq = read_imagef(input, sampler, q).x;
				float2 space = (float2)(p.x - q.x, p.y - q.y);
				float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));
				float range = ip - iq;
				float gr = exp(-sigmaR * (range*range));
				float factor = gs * gr;
				sum += factor * iq;
				norm += factor;
			}
		}
		for (q.x = p.x + halfWindow, q.y = p.y - halfWindow; q.x >= p.x - halfWindow; --q.x, ++q.y) {
			if (q.x >= 0 && q.y >= 0 && q.x <= size.x && q.y <= size.y && q.x != p.x) {
				float iq = read_imagef(input, sampler, q).x;
				float2 space = (float2)(p.x - q.x, p.y - q.y);
				float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));
				float range = ip - iq;
				float gr = exp(-sigmaR * (range*range));
				float factor = gs * gr;
				sum += factor * iq;
				norm += factor;
			}
		}
		float out = sum / norm;
		write_imagef(output, p, (float4)(out, out, out, out));
	}
}


__kernel void multiBilinear(read_only image2d_t input, write_only image2d_t output, const float sigmaS, const float sigmaR, const int halfWindow) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

	const int2 size = get_image_dim(input); // Image size

	int2 q, p = (int2)(get_global_id(0), get_global_id(1));

	if (p.x <= size.x && p.y <= size.y) {
		float4 ip = read_imagef(input, sampler, p);

		float4 sum = 0.0f, norm = 0.0f;

		sum += ip;
		norm += 1.0f;

		q = p;

		for (q.x = p.x - halfWindow; q.x <= p.x + halfWindow; ++q.x) {
			if (q.x >= 0 && q.x <= size.x && q.x != p.x) {
				float4 iq = read_imagef(input, sampler, q);

				float2 space = (float2)(p.x - q.x, p.y - q.y);
				float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));

				float4 range = ip - iq;
				float4 gr = exp(-sigmaR * (range*range));

				float4 factor = gs * gr;

				sum += factor * iq;
				norm += factor;
			}
		}

		q = p;

		for (q.y = p.y - halfWindow; q.y <= p.y + halfWindow; ++q.y) {
			if (q.y >= 0 && q.y <= size.y && q.y != p.y) {
				float4 iq = read_imagef(input, sampler, q);

				float2 space = (float2)(p.x - q.x, p.y - q.y);
				float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));

				float4 range = ip - iq;
				float4 gr = exp(-sigmaR * (range*range));

				float4 factor = gs * gr;

				sum += factor * iq;
				norm += factor;
			}
		}

		for (q.x = p.x - halfWindow, q.y = p.y - halfWindow; q.x <= p.x + halfWindow; ++q.x, ++q.y) {
			if (q.x >= 0 && q.y >= 0 && q.x <= size.x && q.y <= size.y && q.x != p.x) {
				float4 iq = read_imagef(input, sampler, q);

				float2 space = (float2)(p.x - q.x, p.y - q.y);
				float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));

				float4 range = ip - iq;
				float4 gr = exp(-sigmaR * (range*range));

				float4 factor = gs * gr;

				sum += factor * iq;
				norm += factor;
			}
		}

		for (q.x = p.x + halfWindow, q.y = p.y - halfWindow; q.x >= p.x - halfWindow; --q.x, ++q.y) {
			if (q.x >= 0 && q.y >= 0 && q.x <= size.x && q.y <= size.y && q.x != p.x) {
				float4 iq = read_imagef(input, sampler, q);

				float2 space = (float2)(p.x - q.x, p.y - q.y);
				float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));

				float4 range = ip - iq;
				float4 gr = exp(-sigmaR * (range*range));

				float4 factor = gs * gr;

				sum += factor * iq;
				norm += factor;
			}
		}

		float4 out = sum / norm;
		write_imagef(output, p, (float4)(out.x, out.y, out.z, 1.0f));
	}
}



__kernel void biFixed(read_only image2d_t input, write_only image2d_t output, float sigmaS, float sigmaR, int halfWindow) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

	const int2 size = get_image_dim(input); // Image size

	int2 q, p = (int2)(get_global_id(0), get_global_id(1));

	int fixed_array[121] = { 
		0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
		0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0,
		0, 1, 1, 2, 2, 2, 2, 2, 1, 1, 0,
		0, 1, 2, 3, 3, 3, 3, 3, 2, 1, 0,
		1, 2, 2, 3, 4, 4, 4, 3, 2, 2, 1,
		1, 2, 2, 3, 4, 5, 4, 3, 2, 2, 1,
		1, 2, 2, 3, 4, 4, 4, 3, 2, 2, 1,
		0, 1, 2, 3, 3, 3, 3, 3, 2, 1, 0,
		0, 1, 1, 2, 2, 2, 2, 2, 1, 1, 0,
		0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0,
		0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0
	};
	float koef[6] = { 0.0f, 0.015625f, 0.0625f, 0.125f, 0.5f, 1.0f };

	if (p.x <= size.x && p.y <= size.y) {
		float ip = read_imagef(input, sampler, p).x;

		float factor;
		float gs, gr, rangeR;

		float sum = 0.0f, norm = 0.0f;

		int row = p.y;
		int col = p.x;
		int height = size.y;
		int width = size.x;

		for (int y = -5; y < 6; y++) {
			if ((row + y) > 0 && (row + y) < height) {
				for (int x = -5; x < 6; x++) {
					if ((col + x) > 0 && (col + x) < width) {

						gs = koef[fixed_array[(y + 5) * 11 + x + 5]];
						float iq = read_imagef(input, sampler, (int2)(col + x, row + y)).x;
						rangeR = iq - ip;

						gr = 0.00;
						if (rangeR < 1.087f){
							gr = 0.125f;
						}
						if (rangeR < 0.9148f){
							gr = 0.25f;
						}
						if (rangeR < 0.70029f){
							gr = 0.5f;
						}
						if (rangeR < 0.37926){
							gr = 1.0;
						}

						factor = gs * gr;
						sum += factor * iq;
						norm += factor;
					}
				}
			}
		}

		float out = sum / norm;
		write_imagef(output, p, (float4)(out, out, out, out));
	}
}



__kernel void biFixedSmaller(read_only image2d_t input, write_only image2d_t output, float sigmaS, float sigmaR, int halfWindow) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

	const int2 size = get_image_dim(input); // Image size

	int2 q, p = (int2)(get_global_id(0), get_global_id(1));

	int fixed_array[121] = {
		0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
		0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0,
		0, 1, 1, 2, 2, 2, 2, 2, 1, 1, 0,
		0, 1, 2, 3, 3, 3, 3, 3, 2, 1, 0,
		1, 2, 2, 3, 4, 4, 4, 3, 2, 2, 1,
		1, 2, 2, 3, 4, 5, 4, 3, 2, 2, 1,
		1, 2, 2, 3, 4, 4, 4, 3, 2, 2, 1,
		0, 1, 2, 3, 3, 3, 3, 3, 2, 1, 0,
		0, 1, 1, 2, 2, 2, 2, 2, 1, 1, 0,
		0, 0, 1, 1, 2, 2, 2, 1, 1, 0, 0,
		0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0
	};
	// koef[1] could pot. be 0.125f too
	float koef[6] = { 0.0f, 0.0f, 0.125f, 0.125f, 0.5f, 1.0f };

	if (p.x <= size.x && p.y <= size.y) {
		float ip = read_imagef(input, sampler, p).x;

		float factor;
		float gs, gr, rangeR;

		float sum = 0.0f, norm = 0.0f;

		int row = p.y;
		int col = p.x;
		int height = size.y;
		int width = size.x;

		for (int y = -5; y < 6; y++) {
			if ((row + y) > 0 && (row + y) < height) {
				for (int x = -5; x < 6; x++) {
					if ((col + x) > 0 && (col + x) < width) {

						gs = koef[fixed_array[(y + 5) * 11 + x + 5]];
						float iq = read_imagef(input, sampler, (int2)(col + x, row + y)).x;
						rangeR = iq - ip;

						gr = 0.00;
						if (rangeR < 1.087f){
							gr = 0.125f;
						}
						if (rangeR < 0.9148f){
							gr = 0.25f;
						}
						if (rangeR < 0.70029f){
							gr = 0.5f;
						}
						if (rangeR < 0.37926){
							gr = 1.0;
						}

						factor = gs * gr;
						sum += factor * iq;
						norm += factor;
					}
				}
			}
		}

		float out = sum / norm;

		short out2 = (short)(256.0f * out);

		float out3 = (float)out2 / 256.0f;

		write_imagef(output, p, (float4)(out3, out3, out3, out3));
	}
}
