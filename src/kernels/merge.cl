__constant float4 rgb2yuv[] = {
//	(float4)(0.2126f, 0.7152f, 0.0722f, 0.0f)
	(float4)(0.3333f, 0.3333f, 0.3333f, 0.0f)
};

__constant float invgam = 2.2f;

float4 readInside(read_only image2d_t image, sampler_t sampler, int2 coord) {
	int2 dim = get_image_dim(image);
	if (coord.x >= 0 && coord.y >= 0 && coord.x < dim.x && coord.y < dim.y) {
		return read_imagef(image, sampler, coord);
	} else {
		return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
	}
}

float luma(float4 p) {
	return clamp(dot(p, rgb2yuv[0]), 0.0f, 1.0f);
	//return (p.s0 + p.s1 + p.s2) / 3.0f;
}

float hat(float l) {
	return clamp(1.0f - pow(fabs((2.0f * l) - 1.0f), 12.0f), 0.0f, 1.0f);
}
/*
float hat4(float4 pixel) {
	float red = 10.0f * pow(pixel.x, 2.2) * (1.0f - pow((2.0f * pixel.x) - 1.0f, 12.0f));
	float green = 10.0f * pow(pixel.y, 2.2) * (1.0f - pow((2.0f * pixel.y) - 1.0f, 12.0f));
	float blue = 10.0f * pow(pixel.z, 2.2) * (1.0f - pow((2.0f * pixel.z) - 1.0f, 12.0f));
	return blue < green ? (blue < red ? blue : red) : (green < red ? green : red);
}*/


__kernel void merge4(write_only image2d_t targetFrame, read_only image2d_t alphaFrame, read_only image2d_t betaFrame, read_only image2d_t gammaFrame, read_only image2d_t deltaFrame, __global float * isos, int2 alphaOffset, int2 betaOffset, int mask) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);
	int hOffset = get_global_offset(0), vOffset = get_global_offset(1);
	const float invgam = 2.2f;

	float4 alpha = readInside(alphaFrame, sampler, (int2)(x, y) + alphaOffset);
	float4 beta  = readInside(betaFrame,  sampler, (int2)(x, y) + alphaOffset);
	float4 gamma = readInside(gammaFrame, sampler, (int2)(x, y) + betaOffset);
	float4 delta = readInside(deltaFrame, sampler, (int2)(x, y) + betaOffset);

	alpha = pow(alpha, invgam);
	beta  = pow(beta,  invgam);
	gamma = pow(gamma, invgam);
	delta = pow(delta, invgam);

	float lAlpha = luma(alpha);
	float lBeta  = luma(beta);
	float lGamma = luma(gamma);
	float lDelta = luma(delta);

	float4 target = (float4)(0.0f, 0.0f, 0.0f, 1.0f);

	if (lAlpha < 0.000004f) {target = alpha * pow(2, isos[0]);}
	else if (lDelta > (1-0.000004f)) {target = delta * pow(2, isos[3]);}
	else {
		float hatAlpha = hat(lAlpha);
		float hatBeta  = hat(lBeta);
		float hatGamma = hat(lGamma);
		float hatDelta = hat(lDelta);

		float divide = (hatAlpha + hatBeta + hatGamma + hatDelta);

		hatAlpha = hatAlpha / divide;
		hatBeta = hatBeta / divide;
		hatGamma = hatGamma / divide;
		hatDelta = hatDelta / divide;

		if (mask > 50) {
			if (mask > 51) {
				if (mask > 52) {
					if (mask > 53) {
						delta = (float4)(isos[3],0,0,0);
					} else {
						gamma = (float4)(isos[2],0,0,0);
					}
				} else {
					beta = (float4)(isos[1],0,0,0);
				}
			} else {
				alpha = (float4)(isos[0],0,0,0);
			}
		}

		target = (
				hatAlpha * alpha * pow(2, isos[0]) + 
				hatBeta  * beta  * pow(2, isos[1]) + 
				hatGamma * gamma * pow(2, isos[2]) + 
				hatDelta * delta * pow(2, isos[3]));

	}
	
	write_imagef(targetFrame, (int2)(x - hOffset, y - vOffset), (float4)(target.s0, target.s1, target.s2, 1.0f));
}

/*
__kernel void merge4(write_only image2d_t targetFrame,	read_only image2d_t alphaFrame, read_only image2d_t betaFrame, 
														read_only image2d_t gammaFrame, read_only image2d_t deltaFrame, 
														__global float * isos, 
														int2 alphaOffset, int2 betaOffset, 
														int mask) {
	int x = get_global_id(0), y = get_global_id(1);
	int hOffset = get_global_offset(0), vOffset = get_global_offset(1);

	float4 alpha = readInside(alphaFrame, sampler, (int2)(x, y) + alphaOffset);
	float4 beta  = readInside(betaFrame,  sampler, (int2)(x, y) + alphaOffset);
	float4 gamma = readInside(gammaFrame, sampler, (int2)(x, y) + betaOffset);
	float4 delta = readInside(deltaFrame, sampler, (int2)(x, y) + betaOffset);
	
	float lAlpha = luma(alpha);
	float lBeta  = luma(beta);
	float lGamma = luma(gamma);
	float lDelta = luma(delta);

	float4 target = (float4)(0.0f, 0.0f, 0.0f, 1.0f);

	if (lAlpha < 1e-2f) {target = pow(pow(alpha, 1/invgam) * isos[0], 1.0f)/(pow(isos[0], 1.0f)+pow(isos[1], 1.0f)+pow(isos[2], 1.0f)+pow(isos[3], 1.0f));}
	else if (lDelta > (1-(1e-2f))) {target = pow(pow(delta, 1/invgam) * isos[3], 1.0f)/(pow(isos[0], 1.0f)+pow(isos[1], 1.0f)+pow(isos[2], 1.0f)+pow(isos[3], 1.0f));}
	else {
		float hatAlpha = hat(lAlpha);
		float hatBeta  = hat(lBeta);
		float hatGamma = hat(lGamma);
		float hatDelta = hat(lDelta);

		float divide = (hatAlpha + hatBeta + hatGamma + hatDelta)*(pow(isos[0], 1.0f)+pow(isos[1], 1.0f)+pow(isos[2], 1.0f)+pow(isos[3], 1.0f));

		hatAlpha = hatAlpha / divide;
		hatBeta = hatBeta / divide;
		hatGamma = hatGamma / divide;
		hatDelta = hatDelta / divide;

		if (mask > 50) {
			if (mask > 51) {
				if (mask > 52) {
					if (mask > 53) {
						delta = (float4)(isos[3],0,0,0);
					} else {
						gamma = (float4)(isos[2],0,0,0);
					}
				} else {
					beta = (float4)(isos[1],0,0,0);
				}
			} else {
				alpha = (float4)(isos[0],0,0,0);
			}
		}

		target = (
				hatAlpha * pow(pow(alpha, 1/invgam) * isos[0], 1.0f) + 
				hatBeta * pow(pow(beta, 1/invgam) * isos[1], 1.0f) + 
				hatGamma * pow(pow(gamma, 1/invgam) * isos[2], 1.0f) + 
				hatDelta * pow(pow(delta, 1/invgam) * isos[3], 1.0f));

	}
	target = pow(target, invgam);
	write_imagef(targetFrame, (int2)(x - hOffset, y - vOffset), (float4)(target.s0, target.s1, target.s2, 1.0f));
}*/

float4 rgbyuv(const float4 input) {
    const float y = (input.x*0.299, input.y*0.587, input.z*0.114);
    const float u = (input.x*-0.168736, input.y*-0.331264, input.z*0.5);
    const float v = (input.x*0.5, input.y*-0.418688, input.z*-0.081313);
    return (float4)(y, u, v, 1.0f);
}

float4 yuvrgb(const float4 input) {
    const float r = (input.x*1 + input.y*0 + input.z*1.13983);
    const float g = (input.x*1 + input.y*-0.28886 + input.z*-0.58060);
    const float b = (input.x*1 + input.y*2.03211 + input.z*0);
    return (float4)(r, g, b, 1.0f);
}

__kernel void copyImage(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	const int2 x = (const int2)(get_global_id(0), get_global_id(1));
	write_imagef(output, x, read_imagef(input, sampler, x));
}

__kernel void floatToBGR24(read_only image2d_t input, __global uchar * output, const float gamma) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);
	int2 dim = get_image_dim(input);
	float4 in = read_imagef(input, sampler, (int2)(x, y));
	in = pow(in, gamma);
	in = clamp(in, 0.0f, 1.0f);
	output[(dim.x*y + x) * 3] = (uchar)(in.s2*255.0f);
	output[(dim.x*y + x) * 3 + 1] = (uchar)(in.s1*255.0f);
	output[(dim.x*y + x) * 3 + 2] = (uchar)(in.s0*255.0f);
}

__kernel void bgraToRgba(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);
	float4 inp = read_imagef(input, sampler, (int2)(x, y));
	write_imagef(output, (int2)(x, y), (float4)(inp.s2, inp.s1, inp.s0, inp.s3));
}

__kernel void merge2(write_only image2d_t targetFrame, read_only image2d_t topFrame, read_only image2d_t bottomFrame, __global float * isos) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);
	int hOffset = get_global_offset(0), vOffset = get_global_offset(1);

	float4 top = readInside(topFrame, sampler, (int2)(x, y));
	float4 bottom  = readInside(bottomFrame,  sampler, (int2)(x, y));


//	top = pow(top, 1/invgam);
//	beta  = pow(beta,  1/invgam);

	float lAlpha = luma(top);
	float lBeta  = luma(bottom);

	float4 target = (float4)(0.0f, 0.0f, 0.0f, 1.0f);

	if (lAlpha < 1e-2f) {target = (top * isos[0])/(isos[0]+isos[1]);}
	else if (lBeta > (1-(1e-2f))) {target = (bottom * isos[1])/(isos[0]+isos[1]);}
	else {
		float hatAlpha = hat(lAlpha);
		float hatBeta  = hat(lBeta);

		float divide = (hatAlpha + hatBeta)*(isos[0]+isos[1]);


		target = (
				hatAlpha * top * isos[0] +
				hatBeta  * bottom  * isos[1]);

		target = target / divide;
		//target = (float4)(0, target.s1, 0, 1.0f) / divide;
		
	}

	write_imagef(targetFrame, (int2)(x - hOffset, y - vOffset), (float4)(target.s0, target.s1, target.s2, 1.0f));
	
	}

__kernel void merge3(write_only image2d_t targetFrame, read_only image2d_t topFrame, read_only image2d_t middleFrame, read_only image2d_t bottomFrame, __global float * isos) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int x = get_global_id(0), y = get_global_id(1);
	int hOffset = get_global_offset(0), vOffset = get_global_offset(1);

	float4 top = readInside(bottomFrame, sampler, (int2)(x, y));
	float4 middle = readInside(middleFrame,  sampler, (int2)(x, y));
	float4 bottom = readInside(topFrame,  sampler, (int2)(x, y));

	float lAlpha = luma(top);
	float lBeta  = luma(middle);
	float lGamma = luma(bottom);

	float4 target = (float4)(0.0f, 0.0f, 0.0f, 1.0f);

	if (lAlpha < 1e-2f) {target = (top * isos[0]);}
	else if (lGamma > (1-(1e-2f))) {target = (bottom * isos[2]);}
	else {
		float hatAlpha = hat(lAlpha);
		float hatBeta  = hat(lBeta);
		float hatGamma = hat(lGamma);

		float divide = (hatAlpha + hatBeta + hatGamma);// *(isos[0] + isos[1] + isos[2]);

		target = (
				hatAlpha * top * isos[0] +
				hatBeta  * middle * isos[1] +
				hatGamma * bottom * isos[2]);
		
		target = target / divide;
	}

	write_imagef(targetFrame, (int2)(x - hOffset, y - vOffset), (float4)(target.s0, target.s1, target.s2, 1.0f));
}
