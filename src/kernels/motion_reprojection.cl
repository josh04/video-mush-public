#ifndef MOTION_REPROJECTION_CL
#define MOTION_REPROJECTION_CL

__kernel void motion_reprojection_demo(read_only image2d_t previous, read_only image2d_t motion, read_only image2d_t output_read, write_only image2d_t output) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const int2 dim = get_image_dim(previous);
    
    const float4 m = read_imagef(motion, sampler_nearest, coord);
    
    float4 prev = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    //float4 prev = read_imagef(output_read, sampler_nearest, coord);
    
    const float2 coords_moved = (float2)(coord.x - m.x + 0.5, coord.y - m.y + 0.5);
    
    if (coords_moved.x < (float)dim.x && coords_moved.y < (float)dim.y && coords_moved.x >= 0.0f && coords_moved.y >= 0.0f) {
        prev = read_imagef(previous, sampler_linear, coords_moved);
    }
    
    write_imagef(output, coord, prev);
}

/*

__kernel void motion_reprojection_demo(read_only image2d_t previous, read_only image2d_t motion, read_only image2d_t output_read, write_only image2d_t output) {
    
    
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const int2 dim = get_image_dim(previous);
    
    const float4 m = read_imagef(motion, sampler_nearest, coord);
    
    
    //float4 prev = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 prev = read_imagef(output_read, sampler_nearest, coord);
    
    const float2 coords_moved = (float2)((coord.x - m.x + 0.5)/dim.x, (coord.y - m.y + 0.5)/dim.y);
    
    // const float d = read_imagef(previous_depth, sampler_linear, coords_moved).x;
    // if (fabs(d - m.z) < 0.1f) {
    //if (coords_moved.x < (float)dim.x && coords_moved.y < (float)dim.y && coords_moved.x >= 0.0f && coords_moved.y >= 0.0f) {
    prev = read_imagef(previous, sampler_linear, coords_moved);
    //}
    //}
    
    write_imagef(output, coord, prev);
}
*/

__kernel void motion_preprocess(read_only image2d_t motion, write_only image2d_t output) {
	const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;

	const int2 coord = (int2)(get_global_id(0), get_global_id(1));

	const float2 dim = (float2) { get_image_dim(motion).x, get_image_dim(motion).y };

	float4 m = read_imagef(motion, sampler_nearest, coord);
/*
	if (m.x > dim.x / 2.0f) {
		m.x = m.x - dim.x;
	} else if (m.x < -dim.x / 2.0f) {
		m.x = m.x + dim.x;
	}
*/
	write_imagef(output, coord, m);

}

__kernel void motion_reprojection_demo_with_depth(read_only image2d_t previous, read_only image2d_t motion, read_only image2d_t output_read, write_only image2d_t output, read_only image2d_t previous_depth) {
    const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    const sampler_t sampler_linear = CLK_FILTER_LINEAR | CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const int2 dim = get_image_dim(previous);
    
    const float4 m = read_imagef(motion, sampler_nearest, coord);
    

    //float4 prev = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    //float4 prev = (float4)(1.0f, 0.0f, 0.0f, 1.0f);//read_imagef(output_read, sampler_nearest, coord);
    float4 prev = read_imagef(output_read, sampler_nearest, coord);
    
    const float2 coords_moved = clamp((float2)((coord.x - m.x + 0.5)/dim.x, (coord.y - m.y + 0.5)/dim.y), 0.0f, 1.0f);
    
    
    
    const float d = read_imagef(previous_depth, sampler_linear, coords_moved).x;
    //if (fabs(log(d) - log(m.z)) < 0.01f && !(d > 10000.0f && m.z > 10000.0f) ) {
    if (fabs(log(d) - log(m.z)) < 0.01f ) {
    //if (coords_moved.x < 1.0f && coords_moved.y < 1.0f && coords_moved.x >= 0.0f && coords_moved.y >= 0.0f) {
        //prev = (float4)(0.0f, 1.0f, 0.0f, 1.0f);// = read_imagef(previous, sampler_linear, coords_moved);
        prev = read_imagef(previous, sampler_linear, coords_moved);
    //}
    }
    
    write_imagef(output, coord, prev);
}

struct motion_ray {
	float4 origin;
	float4 dir;
};

struct motion_ray par_perspective_get_ray(const float x, const float y, float4 camera_location, float4 top_left, float4 dx, float4 dy) {
	struct motion_ray r;
	//update_matrices();
	float4 rayDir = top_left - dx*x - dy*y;

	r.origin = camera_location;
	r.dir = normalize(rayDir);
	return r;
}

float4 par_perspective_rotate_y(float4 dir, float angle) {
	const float4 row1 = (float4)(cos(angle),	 0,		sin(angle), 0.0f);
	const float4 row2 = (float4)(0,			 1,		0,			0.0f);
	const float4 row3 = (float4)(-sin(angle),	 0,		cos(angle), 0.0f);

	return (float4){ dir.x * row1.x + dir.y * row1.y + dir.z * row1.z,
		dir.x * row2.x + dir.y * row2.y + dir.z * row2.z,
		dir.x * row3.x + dir.y * row3.y + dir.z * row3.z,
		 0.0f
	};
		
}

float4 par_perspective_rotate_z(float4 dir, float angle) {
	const float4 row1 = (float4)(cos(angle),	 -sin(angle),	0, 0.0f);
	const float4 row2 = (float4)(sin(angle),	 cos(angle),	0, 0.0f);
	const float4 row3 = (float4)(0,				 0,				1, 0.0f);

	return (float4){ dir.x * row1.x + dir.y * row1.y + dir.z * row1.z,
		dir.x * row2.x + dir.y * row2.y + dir.z * row2.z,
		dir.x * row3.x + dir.y * row3.y + dir.z * row3.z,
		0.0f
	};
}

float4 par_perspective_get_motion_u_v(const struct motion_ray ray, const float depth, const int width, const int height, const float4 motion_translate, const float motion_old_theta, const float motion_old_phi, const float cwidth, const float cheight)  {

	/*

	// kk so these two are about object motion, lets stick to camera motion for now

	motion3d reloc = props.intersectedShape->getMotion();

	const point3d relocatedPointOnShape = ((props.pointOnShape+reloc.translate) - (props.intersectedShape->getLocation()+reloc.translate))*reloc.rotate + (props.intersectedShape->getLocation() + reloc.translate);

	// so relocatedPointOnShape is just the point where the ray hits, for now???
	*/

	int w = width;// -XOFF * 2;
	int h = height;// -YOFF * 2;

	float4 hitPoint = ray.origin + ray.dir * depth;
	float4 dir = (hitPoint - (motion_translate));

	float4 transDir = dir;
	transDir = par_perspective_rotate_y(transDir, -(motion_old_theta*(M_PI / 180.0)));
	transDir = par_perspective_rotate_z(transDir, -(motion_old_phi*(M_PI / 180.0)));


	float4 old_cam_dir = (float4)(1.0, 0.0, 0.0, 0.0);
	old_cam_dir = par_perspective_rotate_y(old_cam_dir, -(motion_old_theta*(M_PI / 180.0)));
	old_cam_dir = par_perspective_rotate_z(old_cam_dir, -(motion_old_phi*(M_PI / 180.0)));

	float check_direction = dot(old_cam_dir, transDir);
	/*if (check_direction < 0.0f) {
		return (float4) { 0.0f, 0.0f, 0.0f, 1.0f };
	}*/
	//transDir.RotateY(-(motion.oldTheta*(M_PI / 180.0)));
	//transDir.RotateZ(-(motion.oldPhi*(M_PI / 180.0)));

	float rd = length(transDir);

	float div = transDir.x;///scene->camera->cheight;

	transDir = transDir / div;

	float u = (((transDir.z + cwidth) / (2.0 * cwidth)) * w);
	float v = (((-transDir.y + cheight) / (2.0 * cheight)) * h);

	return (float4){ u, v, rd, 1.0f };
}

struct motion_ray par_360_get_ray(const float x, const float y, float4 camera_location, float x_mul, float x_add, float y_mul) {
	struct motion_ray r;
	//update_matrices();

	float xz = x_mul * x + x_add;
	float yz = y_mul * y;

	//rotDY.RotateY(xz);
	//rotDX.RotateZ(yz);
	float4 camera_up = (float4)(0.0, 1.0f, 0.0f, 0.0f);
	float4 rayDir = par_perspective_rotate_y(par_perspective_rotate_z(camera_up, yz), xz);

	r.origin = camera_location;
	r.dir = normalize(rayDir);
	return r;
}


float4 par_360_get_motion_u_v(const struct motion_ray ray, const float depth, const int width, const int height, const float4 motion_translate, const float motion_old_theta, const float motion_old_phi) {


	float w = width;// -XOFF * 2;
	float h = height;// -YOFF * 2;

	float4 hitPoint = ray.origin + ray.dir * depth;
	float4 dir = (hitPoint - (motion_translate));

	float4 transDir = dir;

	float rd = length(transDir);
	float div = transDir.x;///scene->camera->cheight;


	float xang = atan2(transDir.z, -transDir.x) + M_PI;

	float yang;
	if (transDir.y > 0.0f) {
		yang = atan(sqrt(transDir.x * transDir.x + transDir.z * transDir.z) / transDir.y);
	} else if (transDir.y < 0.0f) {
		yang = M_PI + atan(sqrt(transDir.x * transDir.x + transDir.z * transDir.z) / transDir.y);
	} else {
		yang = M_PI_2;
	}
	/*
	if (isnan(yang)) {

	yang = M_PI_2;
	}
	*/

	float x_add_old = (motion_old_theta * (M_PI / 180.0f));

	float x_mod = xang - x_add_old;
	float blah = x_mod - M_PI * 2.0f * floor(x_mod / M_PI / 2.0f);

	float u = blah * w / (2.0f * M_PI); // no x_add b/c we're accounting for that in oldTheta
	float v = (yang)* h / M_PI;

	return (float4) { u, v, rd, 1.0f };

}


__kernel void generate_motion_from_depth(read_only image2d_t depth_in, write_only image2d_t motion_out, float4 camera_location, float4 top_left, float4 dx, float4 dy, const float4 motion_translate, const float motion_old_theta, const float motion_old_phi, const float cwidth, const float cheight, const int source_is_spherical) {
	const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	const int2 dim = get_image_dim(motion_out);
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	//auto d = _depth_host[j * _width + i];
	float d = read_imagef(depth_in, sampler_nearest, coord).x;

	struct motion_ray r = par_perspective_get_ray((float)coord.x + 0.5f, (float)coord.y + 0.5f, camera_location, top_left, dx, dy);

	//if (_is_360) {
	//	_par->getRay((float)i + 0.5f, (float)j + 0.5f, 0.0f, 0.0f, r);
	//} else {
	//	_par->getRay((float)i + 0.5f, (float)j + 0.5f, 0.0f, 0.0f, r);
	//}

	float4 get_motion;
	if (source_is_spherical) {
		get_motion = par_360_get_motion_u_v(r, d, dim.x, dim.y, motion_translate, motion_old_theta, motion_old_phi);
	} else {
		get_motion = par_perspective_get_motion_u_v(r, d, dim.x, dim.y, motion_translate, motion_old_theta, motion_old_phi, cwidth, cheight);
	}
	float u = get_motion.x;
	float v = get_motion.y;
	const float de = get_motion.z;


	/*
	if (_is_360) {
		u = (float)i + 0.5f - u;
		v = (float)j + 0.5f - v;
		_motion_host[j * _width + i] = { (float)u, (float)v, (float)de, 1.0f };
	} else {*/
		u = (float)coord.x + 0.5f - u;
		v = (float)coord.y + 0.5f - v;
		//_motion_host[j * _width + i] = { (float)u, (float)v, (float)de, 1.0f };
		write_imagef(motion_out, coord,  (float4){ (float)u, (float)v, (float)de, 1.0f });
	//}


}

__kernel void generate_motion_from_depth_360(read_only image2d_t depth_in, write_only image2d_t motion_out, float4 camera_location, float x_mul, float x_add, float y_mul, const float4 motion_translate, const float motion_old_theta, const float motion_old_phi) {
	const sampler_t sampler_nearest = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	const int2 dim = get_image_dim(motion_out);
	const int2 coord = (int2)(get_global_id(0), get_global_id(1));

	float d = read_imagef(depth_in, sampler_nearest, coord).x;

	struct motion_ray r = par_360_get_ray((float)coord.x + 0.5f, (float)coord.y + 0.5f, camera_location, x_mul, x_add, y_mul);

	const float4 get_motion = par_360_get_motion_u_v(r, d, dim.x, dim.y, motion_translate, motion_old_theta, motion_old_phi);
    
	float u = get_motion.x;
	float v = get_motion.y;
	const float de = get_motion.z;

	u = (float)coord.x + 0.5f - u;
	v = (float)coord.y + 0.5f - v;
	write_imagef(motion_out, coord, (float4) { (float)u, (float)v, (float)de, 1.0f });
}





#endif
