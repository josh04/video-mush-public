//
//  slic.cl
//  video-mush
//
//  Created by Josh McNamee on 12/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_slic_cl
#define media_encoder_slic_cl

#pragma OPENCL EXTENSION cl_khr_fp64: enable

float3 rgbtoxyz(const float3 rgb) {
    const double x = 0.4124564*rgb.s0 + 0.3575761*rgb.s1 + 0.1804375*rgb.s2;
    const double y = 0.2126729*rgb.s0 + 0.7151522*rgb.s1 + 0.0721750*rgb.s2;
    const double z = 0.0193339*rgb.s0 + 0.1191920*rgb.s1 + 0.9503041*rgb.s2;
    return (float3)(x,y,z);
}

float cielab_ab(const float div) {
    float ret;
    if (div > 0.008856f) {
        ret = pow(div, 1.0f/3.0f);
    } else {
        ret = 7.787f * div + 16.0f/116.0f;
    }
    return ret;
}

float3 xyztocielab(const float3 xyz) {
    const float3 whitepoint = (float3)(0.9505, 1.0, 1.0890);
    const float whitediv = xyz.y/whitepoint.y;
    
    const float L = 116.0 * cielab_ab(whitediv) - 16.0f;
    
    
    const float a = 500.0 * (cielab_ab(xyz.x/whitepoint.x) - cielab_ab(whitediv));
    const float b = 200.0 * (cielab_ab(whitediv) - cielab_ab(xyz.z/whitepoint.z));
    
    return (float3)(L, a, b);
}

float3 cielabtoxyz(const float3 lab) {
    const float P = (lab.s0+16.0f)/116.0f;
    
    const float3 whitepoint = (float3)(0.9505, 1.0, 1.0890);
    
    float3 xyz;
    
    const double x_first = pow(P + lab.s1/500.0f, 3.0f);
    
    if (x_first - 0.008856 > 0) {
        xyz.x = whitepoint.x * x_first;
    } else {
        xyz.x = whitepoint.x * (116.0f*(P + lab.s1/500.0f) - 16.0f)/903.3;
    }
    
    const double y_first = pow(P, 3.0f);
    // 7.9996248 = 0.008856*903.3
    if (y_first - 0.008856 > 0) {
        xyz.y = whitepoint.y * y_first;
    } else {
        xyz.y = whitepoint.y * (116.0f*P - 16.0f)/903.3;
    }
    
    const double z_first = pow(P - lab.s2/200.0f, 3.0f);
    
    if (z_first - 0.008856 > 0) {
        xyz.z = whitepoint.z * z_first;
    } else {
        xyz.z = whitepoint.z * (116.0f*(P - lab.s2/200.0f) - 16.0f)/903.3;
    }
    
    return xyz;
}

float3 xyztorgb(float3 xyz) {
    const double r = 3.240479*xyz.s0 + -1.537150*xyz.s1 + -0.498535*xyz.s2;
    const double g = -0.969256*xyz.s0 + 1.875992*xyz.s1 + 0.041556*xyz.s2;
    const double b = 0.055648*xyz.s0 + -0.204043*xyz.s1 + 1.057311*xyz.s2;
    return (float3)(r,g,b);
}
/*
__kernel void slic_convert_test(read_only image2d_t inputImage, write_only image2d_t outputImage) {
	int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    float4 colour = read_imagef(inputImage, sampler, coord);
    
    float3 col = (float3)(colour.s0, colour.s1, colour.s2);
    
    col = xyztocielab(rgbtoxyz(xyztorgb(rgbtoxyz(col))));
    
    write_imagef(outputImage, coord, (float4)(col.s0, col.s1, col.s2, 1.0f));
}
*/
__kernel void slic_rgb_to_cielab(read_only image2d_t inputImage, write_only image2d_t outputImage) {
	int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    float4 colour = read_imagef(inputImage, sampler, coord);
    
    float3 col = (float3)(colour.s0, colour.s1, colour.s2);
    
    col = xyztocielab(rgbtoxyz(col));
    
    write_imagef(outputImage, coord, (float4)(col.s0, col.s1, col.s2, 1.0f));
}

__kernel void slic_cielab_to_rgb(read_only image2d_t inputImage, write_only image2d_t outputImage) {
	int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    float4 colour = read_imagef(inputImage, sampler, coord);
    
    float3 col = (float3)(colour.s0, colour.s1, colour.s2);
    
    col = xyztorgb(cielabtoxyz(col));
    
    write_imagef(outputImage, coord, (float4)(col.s0, col.s1, col.s2, 1.0f));
}

__kernel void slic_clear_clusters(__global int * clusters) {
	int x = get_global_id(0), y = get_global_id(1);
    int width = get_global_size(0);
    
    clusters[width*y+x] = -1;
}

__kernel void slic_clear_distances(__global float * distances) {
	int x = get_global_id(0), y = get_global_id(1);
    int width = get_global_size(0);
    
    distances[width*y+x] = MAXFLOAT;
}

//#if 0

/* this bit copied wholesale */
int2 slic_init_local_minimum(read_only image2d_t img, int2 center) {
    double min_grad = MAXFLOAT;
    int2 loc_min = center;
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    for (int i = center.x-1; i < center.x+2; i++) {
        for (int j = center.y-1; j < center.y+2; j++) {
            float4 c1 = read_imagef(img, sampler, (int2)(j+1, i)); // (image, j+1, i);
            float4 c2 = read_imagef(img, sampler, (int2)(j, i+1)); // cvGet2D(image, j, i+1);
            float4 c3 = read_imagef(img, sampler, (int2)(j, i)); // cvGet2D(image, j, i);
            /* Convert colour values to grayscale values. */
            /*float i1 = c1.s0;
            float i2 = c2.s0;
            float i3 = c3.s0;*/
             float i1 = c1.s0 * 0.11 + c1.s1 * 0.59 + c1.s2 * 0.3;
             float i2 = c2.s0 * 0.11 + c2.s1 * 0.59 + c2.s2 * 0.3;
             float i3 = c3.s0 * 0.11 + c3.s1 * 0.59 + c3.s2 * 0.3;
            
            /* Compute horizontal and vertical gradients and keep track of the
             minimum. */

			 // JOSH04 SQRT FIXME
            if (sqrt(pow(i1 - i3, 2.0f)) + sqrt(pow(i2 - i3, 2.0f)) < min_grad) {
                min_grad = fabs(i1 - i3) + fabs(i2 - i3);
                loc_min.x = i;
                loc_min.y = j;
            }
        }
    }
    
    return loc_min;
}

//#endif

__kernel void slic_init_centers_counts(read_only image2d_t inputImage, __global float3 * centers_col, __global int2 * centers_loc, __global int * center_counts, int step) {
    int x = get_global_id(0), y = get_global_id(1);
    int width = get_global_size(0);
    
    int2 nc = slic_init_local_minimum(inputImage, (int2)((x+1)*step,(y+1)*step));
    
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    float4 get = read_imagef(inputImage, sampler, nc);
    
    centers_col[width*y+x] = (float3)(get.s0, get.s1, get.s2);
    centers_loc[width*y+x] = nc;
    center_counts[width*y+x] = 0;
}

__kernel void slic_clear_centers_counts( __global float3 * centers_col, __global int2 * centers_loc, __global int * center_counts) {
    int x = get_global_id(0);
    
    centers_col[x] = (float3)(0.0f, 0.0f, 0.0f);
    centers_loc[x] = (int2)(0,0);
    center_counts[x] = 0;
}

float slic_compute_dist(const float3 center_col, const int2 center_loc, int2 pixel, float4 colour, int step, float weight) {

	// JOSH04 SQRT FIXME
    float dc = sqrt(pow(center_col.s0 - colour.s0, 2.0f) + pow(center_col.s1 - colour.s1, 2.0f) + pow(center_col.s2 - colour.s2, 2.0f));
    float ds = sqrt(pow(center_loc.x - (float)pixel.x, 2.0f) + pow(center_loc.y - (float)pixel.y, 2.0f));
    
    return sqrt(pow(dc / (float)weight, 2.0f) + pow(ds / (float)step, 2.0f));
    
    //double w = 1.0 / (pow(ns / nc, 2));
    //return sqrt(dc) + sqrt(ds * w);
}

/*
// float image, int image, float image, float image, int buffer, float image
__kernel void slic_update_allocation(read_only image2d_t              inputImage,
                                    __global write_only int         * clusters,
                                     __global read_write float      * distances,
                                     __global read_only float3      * centers_col,
                                     __global read_only int2        * centers_loc,
                                     int                              step,
                                     int                              weight) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	int center = get_global_id(0);
	int2 dim = get_image_dim(inputImage);
    
    
    // Only compare to pixels in a 2 x step by 2 x step region.
    for (int x = centers_loc[center].x - step; x < centers_loc[center].x + step; x++) {
        for (int y = centers_loc[center].y - step; y < centers_loc[center].y + step; y++) {
            
            if (x >= 0 && x < dim.x && y >= 0 && y < dim.y) {
                float4 colour = read_imagef(inputImage, sampler, (int2)(x, y)); //CvScalar colour = cvGet2D(image, l, k);
                double d = slic_compute_dist(centers_col, centers_loc, center, (int2)(x, y), colour, step, weight); // surely x, y ?!?!?!
                
                // Update cluster allocation if the cluster minimizes the distance.
                if (d < distances[x+y*dim.x]) {
                    distances[x+y*dim.x] = d;
                    clusters[x+y*dim.x] = center;
                }
            }
        }
    }
}*/

__kernel void slic_update_allocation_pixelwise(read_only image2d_t              inputImage,
                                               __global int          * clusters,
                                               __global float                   * distances,
                                               __global float3        * centers_col,
                                               __global int2          * centers_loc,
                                               int                              step,
                                               float                            weight,
                                               int                              num_centers) {
	const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
	
	const int2 dim = get_image_dim(inputImage);
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    const float4 colour = read_imagef(inputImage, sampler, coord);
    
    float dis = distances[coord.x+coord.y*dim.x];
    int clus = 0;
    
    for (int i = 0; i < num_centers; ++i) {
        const int2 center_loc = centers_loc[i];
 //       const float coord_distance = distance((float2)center_loc, (float2)coord);
        if (abs(center_loc.x - coord.x) < step) {
            if (abs(center_loc.y - coord.y) < step) {
 //       if (coord_distance < step) {
            const float3 center_col = centers_col[i];
            
            const double d = slic_compute_dist(center_col, center_loc, coord, colour, step, weight); // surely x, y ?!?!?!
            
            if (d < dis) {
                dis = d;
                clus = i;
            }
//      }
            }
        }
    }
    distances[coord.x+coord.y*dim.x] = dis;
    clusters[coord.x+coord.y*dim.x] = clus;
}

__kernel void slic_update_allocation_pod(read_only image2d_t               inputImage,
                                         __global int          * clusters,
                                         __global float                   * distances,
                                         __global float3       * centers_col,
                                         __global int2          * centers_loc,
                                         int                              step,
                                         float                            weight,
                                         int width, int height, int y_offset, int x_offset) {
    int center = get_global_id(0);
    
    int2 coord = centers_loc[center] + (int2)(y_offset, x_offset);
    
    if (coord.x >= 0 && coord.x < width && coord.y >= 0 && coord.y < height) {
        const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
        
        float4 colour = read_imagef(inputImage, sampler, coord); //CvScalar colour = cvGet2D(image, l, k);
        float d = slic_compute_dist(centers_col[center], centers_loc[center], coord, colour, step, weight);
        
        // Update cluster allocation if the cluster minimizes the distance.
        int num = coord.x+coord.y*width;
        if (d < distances[num]) {
            distances[num] = d;
            clusters[num] = center;
        }
    }
}

/*
__kernel void slic_update_cluster_center_p1(read_only image2d_t           inputImage,
                                    __global read_only int         * clusters,
                                    __global float3      * centers_col,
                                    __global int2      * centers_loc,
                                    __global int        * center_counts) {
    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    int width = get_global_size(0);
    
    int c_id = clusters[coord.x+coord.y*width];
    
    if (c_id != -1) {
        const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
        float4 colour = read_imagef(inputImage, sampler, coord);  //cvGet2D(image, k, j); ALSO SUSPECT
        
        centers_col[c_id].s0 += colour.s0;
        centers_col[c_id].s1 += colour.s1;
        centers_col[c_id].s2 += colour.s2;
        centers_loc[c_id].x += coord.x;
        centers_loc[c_id].y += coord.y;
        
        center_counts[c_id] += 1;
    }
}
*/
__kernel void slic_update_cluster_center_p1_clusterwise(read_only image2d_t           inputImage,
                                            __global int         * clusters,
                                            __global float3      * centers_col,
                                            __global int2      * centers_loc,
                                            __global int        * center_counts) {
    int n = get_global_id(0);
    int2 dim = get_image_dim(inputImage);
    
    float3 col = (float3)(0.0f, 0.0f, 0.0f);
    int2 loc = (int2)(0,0);
    int cnt = 0;
    
    for (int h = 0; h < dim.y; ++h) {
        for (int w = 0; w < dim.x; ++w) {
            if (clusters[w+h*dim.x] == n) {
                const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
                float4 colour = read_imagef(inputImage, sampler, (int2)(w,h));  //cvGet2D(image, k, j); ALSO SUSPECT
                
                col.s0 = col.s0 + colour.s0;
                col.s1 = col.s1 + colour.s1;
                col.s2 = col.s2 + colour.s2;
                loc.x = loc.x + w;
                loc.y = loc.y + h;
                
                //col = col + colour;
                //loc = loc + (int2)(w,h);
                
                cnt += 1;
            }
        }
    }
    centers_col[n] = col;
    centers_loc[n] = loc;
    center_counts[n] = cnt;
}

__kernel void slic_update_cluster_center_p2(__global float3 * centers_col, __global int2 * centers_loc, __global int * center_counts) {
    int x = get_global_id(0);
    float divide = (float)center_counts[x] + 0.001;
    centers_col[x] = (centers_col[x] / divide);
    centers_loc[x] = (int2)((int)((float)centers_loc[x].x / divide), (int)((float)centers_loc[x].y / divide));
}

__constant int dx8[8] = {-1, -1,  0,  1, 1, 1, 0, -1};
__constant int dy8[8] = { 0, -1, -1, -1, 0, 1, 1,  1};

__kernel void slic_draw_centers(__global int2         * centers_loc,
                                 write_only image2d_t           outputImage) {
    
    int c = get_global_id(0);
    int2 dim = get_image_dim(outputImage);
    
    /* Initialize the contour vector and the matrix detailing whether a pixel
	 * is already taken to be a contour. */
    
    /* Go through all the pixels. */
    int x = centers_loc[c].x, y = centers_loc[c].y;
    for (int i = x-3; i < x+3; ++i) {
        for (int j = y-3; j < y+3; ++j){
            if (i < dim.x && i > -1 && j < dim.y && j > -1) {
                write_imagef(outputImage, (int2)(i, j), (float4)(50.0f, 128.0f, 128.0f, 1.0f));
            }
        }
    }

}

//#if 0
__kernel void slic_draw_contours(__global int         * clusters,
                                 write_only image2d_t           outputImage) {
    
    int i = get_global_id(0), j = get_global_id(1);
    int width = get_global_size(0), height = get_global_size(1);
    
    /* Initialize the contour vector and the matrix detailing whether a pixel
	 * is already taken to be a contour. */
    
    /* Go through all the pixels. */
    int nr_p = 0;
    
    /* Compare the pixel to its 8 neighbours. */
    for (int k = 0; k < 8; k++) {
        int x = i + dx8[k], y = j + dy8[k];
        
        if (x >= 0 && x < width && y >= 0 && y < height) {
            if (clusters[i+j*width] != clusters[x+y*width]) {
                nr_p += 1;
            }
        }
    }
    
    /* Add the pixel to the contour list if desired. */
    if (nr_p >= 2) {
        write_imagef(outputImage, (int2)(i,j), (float4)(50.0f, 128.0f, 128.0f, 1.0f));
    }
}


//#if 0
__kernel void slic_draw_cells(__global int          * clusters,
                              __global float3       * centers_col,
                              write_only image2d_t           outputImage) {
    
    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    int width = get_global_size(0);
    
    int c_id = clusters[coord.x+coord.y*width];
    
    //colour = colour*10.0f;
    
    if (!(c_id < 0)) {
        float3 colour = centers_col[c_id];
        write_imagef(outputImage, coord, (float4)(colour.s0, colour.s1, colour.s2, 1.0f));
    }
}

__kernel void slic_square_center_col(__global float3 * centers_col, __global float3 * centers_col_squared) {
    int i = get_global_id(0);
    
    float3 in = centers_col[i];
    float3 out;
    out.s0 = in.s0*in.s0;
    out.s1 = in.s1*in.s1;
    out.s2 = in.s2*in.s2;
    
    centers_col_squared[i] = out;
}

__kernel void slic_square_center_loc_and_convert_to_float(__global int2 * centers_loc, __global float2 * centers_loc_float, __global float2 * centers_loc_squared) {
    const int i = get_global_id(0);
    
    const int2 in = centers_loc[i];
    
    centers_loc_float[i] = (float2)((float)in.x, (float)in.y);
    
	float2 out;
	out.s0 = in.s0*in.s0;
	out.s1 = in.s1*in.s1;
    
    centers_loc_squared[i] = out;
}

__kernel void slic_uniqueness(__global float3 * centers_col, __global float3 * centers_col_squared,
                                     __global float3 * centers_col_blur, __global float3 * centers_col_squared_blur,
                                     __global float * uniqueness) {
    const int i = get_global_id(0);
    
    const float3 ci2 = centers_col_squared[i];
    const float3 ci = centers_col[i];
    const float3 blurci = centers_col_blur[i];
    const float3 blurci2 = centers_col_squared_blur[i];
    
    const float3 out = ci2 - (float3)(2.0f, 2.0f, 2.0f)*ci*blurci + blurci2;
    
	// JOSH04 SQRT FIXME
    uniqueness[i] = sqrt(pow(out.s0, 2.0f)+pow(out.s1, 2.0f)+pow(out.s2, 2.0f));
    //uniqueness[i] = (float)i/255.0f;
//    uniqueness[i] = sqrt(powf(ci.s0, 2.0f)+powf(ci.s1, 2.0f)+powf(ci.s2, 2.0f));
}

__kernel void slic_draw_map(__global int          * clusters,
                            __global float       * uniqueness,
                            write_only image2d_t           outputImage,
                            const int flag) {
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int width = get_global_size(0);
    
    const int c_id = clusters[coord.x+coord.y*width];
    
    if (!(c_id < 0)) {
        float colour;
        if (flag) {
            colour = 1.0f - uniqueness[c_id];
        } else {
            
            colour = uniqueness[c_id];
        }
        
        write_imagef(outputImage, coord, (float4)(colour, colour, colour, 1.0f));
    }
}

__kernel void slic_distribution(__global float2 * centers_loc_blur, __global float2 * centers_loc_squared,
                              __global float * distribution) {
    const int i = get_global_id(0);
    
    const float2 li = centers_loc_blur[i];
    const float2 li2 = centers_loc_squared[i];
    
    const float2 out = (float2)(li.x*li.x, li.y*li.y) - li2;

	// JOSH04 SQRT FIXME
    distribution[i] = sqrt(pow(out.s0, 2.0f)+pow(out.s1, 2.0f));
    //uniqueness[i] = (float)i/255.0f;
    //    uniqueness[i] = sqrt(powf(ci.s0, 2.0f)+powf(ci.s1, 2.0f)+powf(ci.s2, 2.0f));
}

__kernel void slic_normalise(__global float * input, float big) {
    const int id = get_global_id(0);
    input[id] = input[id]/big;
}


__kernel void slic_saliency(__global float * uniqueness, __global float * distribution, __global float * saliency) {
    const int id = get_global_id(0);
    
    const float unique = uniqueness[id];
    const float dist = distribution[id];
    
    saliency[id] = unique * exp(-2.0f * dist);
    
}

#endif


