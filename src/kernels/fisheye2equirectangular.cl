#ifndef FISHEYE_2_EQUIRECTANGULAR
#define FISHEYE_2_EQUIRECTANGULAR

// Fisheye to spherical conversion
// Assumes the fisheye image is square, centered, and the circle fills the image.
// Output (spherical) image should have 2:1 aspect.
// Strange (but helpful) that atan() == atan2(), normally they are different.

__kernel void fisheye2equirectangular_sideways(read_only image2d_t input, write_only image2d_t output, float fov_degrees) {
	
	float fov = M_PI * fov_degrees / 180.0f; // FOV of the fisheye, eg: 180 degrees
	
	int2 in_dim = get_image_dim(input);
	int2 out_dim = get_image_dim(output);

	int x = get_global_id(0), y = get_global_id(1);

	// Polar angles
	float theta = 2.0 * M_PI * ((x / (float)out_dim.x) - 0.5); // -pi to pi
	float phi = M_PI * ((y / (float)out_dim.y) - 0.5);	// -pi/2 to pi/2

	// Vector in 3D space
	float3 psph;
	psph.x = cos(phi) * sin(theta);
	psph.y = cos(phi) * cos(theta);
	psph.z = sin(phi);

	float height_mod = 0.5f;

	// Calculate fisheye angle and radius
	theta = atan2(psph.z, psph.x);
	phi = atan2(sqrt(psph.x*psph.x + psph.z*psph.z), psph.y);
	float r = height_mod * phi / fov; // JOSH changed to height from width
    
    float aspect  = in_dim.y / (float)in_dim.x;
    
	// Pixel in fisheye space
	float2 fish_coord;
	fish_coord.x = 0.5 + (r * cos(theta)) * aspect;// -((in_dim.x - in_dim.y) / 2.0f) / in_dim.x;
	fish_coord.y = 0.5 + r * sin(theta);

	const sampler_t sam = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
	float4 inp = read_imagef(input, sam, fish_coord);

	//const sampler_t sam2 = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT;
	//float4 inp = read_imagef(input, sam2, (float2)(x/(float)out_dim.x, y/(float)out_dim.y));

	write_imagef(output, (int2)(x, y), inp);
}
__kernel void fisheye2equirectangular_upwards(read_only image2d_t input, write_only image2d_t output, float fov_degrees, float x_offset, float y_offset) {

	float fov = M_PI * fov_degrees / 180.0f; // FOV of the fisheye, eg: 180 degrees

	int2 in_dim = get_image_dim(input);
	int2 out_dim = get_image_dim(output);

	int x = get_global_id(0), y = get_global_id(1);
    
    float out_adj = out_dim.y * 360.0f/fov_degrees;

	// Polar angles
	float theta = 2.0 * M_PI * ((x / (float)out_dim.x) - 0.5); // -pi to pi
	//float phi = M_PI * ((y / (float)out_dim.y) - 0.5);	// -pi/2 to pi/2

	float height_mod = 1.0f;

	// Calculate fisheye angle and radius
	//theta = atan2(psph.z, psph.x);
	//phi = atan2(sqrt(psph.x*psph.x + psph.z*psph.z), psph.y);
	//float r = height_mod * phi / fov; // JOSH changed to height from width

    
    float aspect  = in_dim.y / (float)in_dim.x;
    
    float r = 0.5 * ((float)y / out_dim.y) *360.0f/fov_degrees;
    
    //float adjust = (360.0f - fov_degrees) / 360.0f;
    
	// Pixel in fisheye space
	float2 fish_coord;
	fish_coord.x = x_offset + (r * cos(theta)) * aspect;// -((in_dim.x - in_dim.y) / 2.0f) / in_dim.x;
	fish_coord.y = y_offset  + r * sin(theta); // 0.5012

	const sampler_t sam = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
	float4 inp = read_imagef(input, sam, fish_coord);

	//const sampler_t sam2 = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT;
	//float4 inp = read_imagef(input, sam2, (float2)(x/(float)out_dim.x, y/(float)out_dim.y));

	write_imagef(output, (int2)(x, y), inp);
}

#endif
