//
//  algebra.cl
//  video-mush
//
//  Created by Josh McNamee on 12/08/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_matrix3d_cl
#define video_mush_matrix3d_cl

// point3d.hpp

#define NULL 0
#define _EPSILON 1e-5
#define _REFLECTIONS 2
#define M_PI    3.14159265358979323846f
#define M_PI_2  1.57079632679489661923132169163975144f

typedef float unit;
typedef float3 point3d;
typedef float4 colour;
typedef float4 packed_sphere;

point3d point3d_unit(const point3d pt) {
	const unit m = length(pt);
	return pt / m;
}

// matrix3d.hpp

typedef struct {
	point3d x;
	point3d y;
	point3d z;
} __attribute__ ((aligned(8))) matrix3d;


matrix3d matrix3d_transpose(const matrix3d trans) {
	matrix3d r;
	r.x = (point3d)(trans.x.x, trans.y.x, trans.z.x);
	r.y = (point3d)(trans.x.y, trans.y.y, trans.z.y);
	r.z = (point3d)(trans.x.z, trans.y.z, trans.z.z);
	return r;
}

matrix3d matrix3d_inverse(const matrix3d i) {
	matrix3d r;
	const unit oneoverdeterminant = 1 / (i.x.x*(i.y.y*i.z.z - i.z.y*i.y.z)
		- i.y.x*(i.x.y*i.z.z - i.z.y*i.x.z)
		+ i.z.x*(i.x.y*i.y.z - i.y.y*i.x.z));

	r.x = (point3d)((i.y.y*i.z.z - i.z.y*i.y.z),
		-(i.y.x*i.z.z - i.z.x*i.y.z),
		(i.y.x*i.z.y - i.z.x*i.y.y))*oneoverdeterminant;
	r.y = (point3d)(-(i.x.y*i.z.z - i.z.y*i.x.z),
		(i.x.x*i.z.z - i.z.x*i.x.z),
		-(i.x.x*i.z.y - i.z.x*i.x.y))*oneoverdeterminant;
	r.z = (point3d)((i.x.y*i.y.z - i.y.y*i.x.z),
		-(i.x.x*i.y.z - i.y.x*i.x.z),
		(i.x.x*i.y.y - i.y.x*i.x.y))*oneoverdeterminant;

	return matrix3d_transpose(r);
}

matrix3d matrix3d_add(matrix3d lhs, const matrix3d rhs) {
	lhs.x = lhs.x + rhs.x;
	lhs.y = lhs.y + rhs.y;
	lhs.z = lhs.z + rhs.z;
	return lhs;
}

matrix3d matrix3d_subtract(matrix3d lhs, const matrix3d rhs) {
	lhs.x = lhs.x - rhs.x;
	lhs.y = lhs.y - rhs.y;
	lhs.z = lhs.z - rhs.z;
	return lhs;
}

matrix3d matrix3d_multiply_double(matrix3d lhs, const unit rhs) {
	lhs.x = lhs.x*rhs;
	lhs.y = lhs.y*rhs;
	lhs.z = lhs.z*rhs;
	return lhs;
}

matrix3d matrix3d_multiply(const matrix3d lhs, const matrix3d rhs) {
	matrix3d r;

	r.x = (point3d)(lhs.x.x*rhs.x.x + lhs.y.x*rhs.x.y + lhs.z.x*rhs.x.z,
		lhs.x.y*rhs.x.x + lhs.y.y*rhs.x.y + lhs.z.y*rhs.x.z,
		lhs.x.z*rhs.x.x + lhs.y.z*rhs.x.y + lhs.z.z*rhs.x.z);
	r.y = (point3d)(lhs.x.x*rhs.y.x + lhs.y.x*rhs.y.y + lhs.z.x*rhs.y.z,
		lhs.x.y*rhs.y.x + lhs.y.y*rhs.y.y + lhs.z.y*rhs.y.z,
		lhs.x.z*rhs.y.x + lhs.y.z*rhs.y.y + lhs.z.z*rhs.y.z);
	r.z = (point3d)(lhs.x.x*rhs.z.x + lhs.y.x*rhs.z.y + lhs.z.x*rhs.z.z,
		lhs.x.y*rhs.z.x + lhs.y.y*rhs.z.y + lhs.z.y*rhs.z.z,
		lhs.x.z*rhs.z.x + lhs.y.z*rhs.z.y + lhs.z.z*rhs.z.z);

	return r;
}

point3d point3d_multiply_matrix3d(const point3d lhs, const matrix3d rhs) {
	point3d r;
	r.x = lhs.x*rhs.x.x + lhs.y*rhs.x.y + lhs.z*rhs.x.z;
	r.y = lhs.x*rhs.y.x + lhs.y*rhs.y.y + lhs.z*rhs.y.z;
	r.z = lhs.x*rhs.z.x + lhs.y*rhs.z.y + lhs.z*rhs.z.z;
	return r;
}

matrix3d zRotation(unit theta) {
	matrix3d r;
	r.x = point3d_unit((point3d)(cos(theta), sin(theta), 0));
	r.y = point3d_unit((point3d)(-sin(theta), cos(theta), 0));
	r.z = (point3d)(0, 0, 1);
	return r;
}

matrix3d xRotation(unit theta) {
	matrix3d r;
	r.x = (point3d)(1, 0, 0);
	r.y = point3d_unit((point3d)(0, cos(theta), sin(theta)));
	r.z = point3d_unit((point3d)(0, -sin(theta), cos(theta)));
	return r;
}

matrix3d yRotation(unit theta) {
	matrix3d r;
	r.x = point3d_unit((point3d)(cos(theta), 0, -sin(theta)));
	r.y = (point3d)(0, 1, 0);
	r.z = point3d_unit((point3d)(sin(theta), 0, cos(theta)));
	return r;
}

// line3d.hpp

typedef struct {
	point3d point;
	point3d direction;
} __attribute__ ((aligned (8))) line3d;

point3d line3d_get_point(const line3d * const line, const unit distance) {
	return line->point + distance*line->direction;
}

// shapes

typedef struct {
	point3d location;
	unit radius;
} __attribute__ ((aligned (32))) sphere;

typedef struct {
    matrix3d coord_transform;
    unit distance;
} __attribute__ ((aligned (8))) plane;


//sphere

bool sphere_intersection(const sphere * const s,
                         const line3d * const l,
                         unit * distance, colour * c) {
    
    const point3d diff = l->point - s->location;
    unit _dot = dot(l->direction, diff);
    unit discriminant = _dot*_dot - dot(diff, diff) + (s->radius * s->radius);

    if (discriminant < 0) {
        return false;
    } else if (discriminant > 0) {
        unit p1 = -_dot + sqrt(discriminant);
        unit p2 = -_dot - sqrt(discriminant);
        *distance = (p1 < p2) ? p1 : p2;
    } else {
        *distance = -_dot;
    }
    *c = (colour)(0.7, 1.0, 0.0, 1.0); // FIXME
    return true;
}

point3d sphere_normal(const sphere * const s, point3d p) {
    p = p - s->location;
    return point3d_unit(p);
}

// plane

plane make_plane_from_angles(const unit distance,
                        const unit theta,
                        const unit phi) {
    plane p;
    p.coord_transform.x = point3d_multiply_matrix3d(point3d_multiply_matrix3d((point3d)(1, 0, 0),zRotation(phi*(M_PI / 180.0))), yRotation(theta*(M_PI / 180.0)));
    p.coord_transform.y = point3d_multiply_matrix3d(point3d_multiply_matrix3d((point3d)(0, 1, 0), zRotation(phi*(M_PI / 180.0))), yRotation(theta*(M_PI / 180.0)));
    p.coord_transform.z = point3d_multiply_matrix3d(point3d_multiply_matrix3d((point3d)(0, 0, 1), zRotation(phi*(M_PI / 180.0))), yRotation(theta*(M_PI / 180.0)));
    p.distance = distance;
    return p;
}

plane make_plane_from_points(const point3d a,
                        const point3d b,
                        const point3d c) {
    const point3d tmp = (b-a);
    const point3d tmp2 = (c-a);
    const point3d norm = cross(tmp, tmp2);
    
    unit theta = (180.0 / M_PI) * atan2(norm.z, norm.x);
    unit phi = (180.0 / M_PI) * atan2(-norm.y, sqrt(pow(norm.x, 2.0f) + pow(norm.z, 2.0f)));
    
    const unit distance = dot(a, norm);
    return make_plane_from_angles(distance, theta, phi);
}

bool plane_intersection(const plane * const p,
                        const line3d * const l,
                        unit * distance, colour * c) {
    const unit t = -(dot(l->point - (p->distance*p->coord_transform.x), p->coord_transform.x)) / dot(l->direction, p->coord_transform.x);
    
    if (t < 0) {
        return false;
    }
    
    *distance = t;
    
    point3d spot = point3d_multiply_matrix3d(line3d_get_point(l, t), p->coord_transform);
    
    *c = (colour)(0.0, 0.0, 0.5, 1.0);
    
    return true;
}

point3d plane_normal(const plane * const p) {
    return p->coord_transform.x;
}

// triangle

typedef struct {
    point3d a;
    point3d transformed_a, transformed_b, transformed_c;
    point3d line_a, line_b, line_c;
    matrix3d coord_transform;
} __attribute__ ((aligned (8))) triangle;

bool triangle_intersection(const triangle * const t,
                           const line3d * const l,
                           unit * distance, colour * c) {
    const unit dist = -(dot(l->point - t->a, t->coord_transform.x)) / dot(l->direction, t->coord_transform.x);
    
    if (t < 0) {
        return false;
    }
    
    const point3d pnt = point3d_multiply_matrix3d(line3d_get_point(l, dist), t->coord_transform);
    
    point3d spot_a = (t->transformed_a - pnt);
    point3d spot_b = (t->transformed_b - pnt);
    point3d spot_c = (t->transformed_c - pnt);
    
    unit check1 = (t->line_a.z * spot_a.y - t->line_a.y * spot_a.z);
    unit check2 = (t->line_b.z * spot_b.y - t->line_b.y * spot_b.z);
    unit check3 = (t->line_c.z * spot_c.y - t->line_c.y * spot_c.z);
    
    //check2 = 1.0;
    //check3 = 1.0;
    
    if (!(check1 >= 0 && check2 >= 0 && check3 >= 0)) {
        return false;
    }
    
    *distance = dist;
    *c = (colour)(0.5f, 0.5f, 0.2f, 1.0f); //texture->colour(spotA.z, spotA.y);
    
    return true;
    
}

triangle make_triangle_from_points(const point3d a,
                                   const point3d b,
                                   const point3d c) {
    const plane p = make_plane_from_points(a, b, c);
    
    triangle t;
    t.a = a;
    t.transformed_a = point3d_multiply_matrix3d(a, p.coord_transform);
    t.transformed_b = point3d_multiply_matrix3d(b, p.coord_transform);
    t.transformed_c = point3d_multiply_matrix3d(c, p.coord_transform);
    t.coord_transform = p.coord_transform;
    t.line_a = point3d_multiply_matrix3d(b-a, p.coord_transform);
    t.line_b = point3d_multiply_matrix3d(c-b, p.coord_transform);
    t.line_c = point3d_multiply_matrix3d(a-c, p.coord_transform);
    
    return t;
}

point3d triangle_normal(const triangle * const t) {
    return t->coord_transform.x;
}

// cell

void _fire_primary(const unsigned int sphere_count,
                   __global packed_sphere * spheres,
                   const unsigned int plane_count,
                   __global plane * planes,
                   const unsigned int triangle_count,
                   __global triangle * triangles,
	const line3d * const l,
	unit * d, colour * c, point3d * n) {

	unit distance = 1e9;
	unit least_distance = 10000.0;
    bool found = false;
    
    point3d norm;

	colour shapeColour = (colour)(0.0, 0.0, 0.0, 1.0);
	colour shapeTemp = (colour)(0.0, 0.0, 0.0, 1.0);

	for (unsigned long i = 0; i < sphere_count; ++i) {
		bool intersects = false;

        const packed_sphere ps = spheres[i];
        const sphere s = { (point3d)(ps.x, ps.y, ps.z), ps.w };
        intersects = sphere_intersection(&s, l, &distance, &shapeTemp);
        
		if (intersects) {
			if (distance < least_distance && distance > _EPSILON) {
				least_distance = distance;
				found = true;
                norm = sphere_normal(&s, line3d_get_point(l, least_distance));
				shapeColour = shapeTemp;
			}
		}
        
        //shapeColour = (colour)(s.radius, 0.0, 0.0, 1.0);
	}
    
    for (unsigned long i = 0; i < plane_count; ++i) {
        bool intersects = false;
        
        const plane p = planes[i];
        intersects = plane_intersection(&p, l, &distance, &shapeTemp);
        
        if (intersects) {
            if (distance < least_distance && distance > _EPSILON) {
                least_distance = distance;
                found = true;
                norm = plane_normal(&p);
                shapeColour = shapeTemp;
            }
        }
    }
    
    for (unsigned long i = 0; i < 1; ++i) {
        bool intersects = false;
        
        const triangle t = triangles[i];
        intersects = triangle_intersection(&t, l, &distance, &shapeTemp);
        intersects = true;
        if (intersects) {
            if (distance < least_distance && distance > _EPSILON) {
                least_distance = distance;
                found = true;
                norm = triangle_normal(&t);
                shapeColour = shapeTemp;
            }
        }
    }

	if (found == false) {
		shapeColour = (colour)(l->direction.x, l->direction.y, l->direction.z, 1.0f);
	}
    
    *n = norm;
	*c = shapeColour;
	*d = least_distance;
}

__kernel void tr_fire_primary(write_only image2d_t  output,
                              write_only image2d_t  depth,
                              write_only image2d_t  geometry,
                              const unsigned int    sphere_count,
                              __global    packed_sphere  * spheres,
                              const unsigned int    plane_count,
                              __global    plane * planes,
                              const unsigned int    triangle_count,
                              __global    triangle * triangles,
                              const                 point3d ray_location,
                              __global    point3d * ray_directions) {
    const unsigned int x = get_global_id(0);
    const unsigned int y = get_global_id(1);
    
    const unsigned int width = get_image_dim(output).x;
    
    const unsigned int i = width*y + x;
    
	const line3d l = { ray_location, ray_directions[i] };

	colour c = { 0.0, 1.0, 0.0, 1.0 };
	unit d = 1e9;
	point3d n = { 1.0, 0.0, 0.0 };

	_fire_primary(sphere_count, spheres, plane_count, planes, triangle_count, triangles, &l, &d, &c, &n);
//	c = (float4)(l.direction.x, l.direction.y, l.direction.z, 1.0f);
    write_imagef(output, (int2)(x, y), c);
    write_imagef(depth, (int2)(x, y), (float4)(d, d, d, 1.0f));
    write_imagef(geometry, (int2)(x, y), (float4)(n.x, n.y, n.z, 1.0f));
}

__kernel void tr_gen_rays(unsigned int width, unsigned int height, point3d gaze, point3d v, point3d u, __global point3d * ray_directions) {
    
    const unsigned int i = get_global_id(0);
    const unsigned int j = get_global_id(1);
    
    point3d dir = gaze + v*(-1.0f + 2.0f*((0.5f + (float)j) / (float)(height))) + u*(-1.0f + 2.0f*((0.5f + (float)i) / (float)(width)));
    
    ray_directions[j*width + i] = point3d_unit(dir);
}

__kernel void tr_gen_triangles(__global  point3d * a_array,
                               __global  point3d * b_array,
                               __global  point3d * c_array,
                               __global  triangle * triangles) {
    const unsigned long triangle_count = get_global_id(0);
    
    const point3d a = a_array[triangle_count];
    const point3d b = b_array[triangle_count];
    const point3d c = c_array[triangle_count];
    
    triangles[triangle_count] = make_triangle_from_points(a, b, c);
}

#endif

