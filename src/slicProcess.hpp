//
//  slicProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_slicProcess_hpp
#define media_encoder_slicProcess_hpp


#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include "profile.hpp"

#include "permutohedral/permutohedral.h"

class slicProcess : public mush::imageProcess {
public:
    slicProcess(mush::config::slicConfigStruct &config, Profile * profile) : mush::imageProcess(), _profile(profile), slicConfig(config) {
        
    }
    
    ~slicProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        
        buffer = castToImage(buffers.begin()[0]);
        
        buffer->getParams(_width, _height, _size);
        
        int number_of_superpixels = 4000;
        weight = 40.0;
        step = (int)(sqrt((_width * _height) / (double) number_of_superpixels));
        
        slic_clear_clusters = context->getKernel("slic_clear_clusters");
        slic_clear_distances = context->getKernel("slic_clear_distances");
        slic_init_centers_counts = context->getKernel("slic_init_centers_counts");
        slic_clear_centers_counts = context->getKernel("slic_clear_centers_counts");
        slic_update_allocation = context->getKernel("slic_update_allocation_pixelwise");
        slic_update_cluster_center_p1 = context->getKernel("slic_update_cluster_center_p1_clusterwise");
        slic_update_cluster_center_p2 = context->getKernel("slic_update_cluster_center_p2");
        slic_draw_contours = context->getKernel("slic_draw_contours");
        slic_draw_centers = context->getKernel("slic_draw_centers");
        slic_draw_cells = context->getKernel("slic_draw_cells");
        
        slic_update_allocation_pod = context->getKernel("slic_update_allocation_pod");
        
        slic_square_center_col = context->getKernel("slic_square_center_col");
        slic_square_center_loc = context->getKernel("slic_square_center_loc_and_convert_to_float");
        slic_uniqueness = context->getKernel("slic_uniqueness");
        slic_distribution = context->getKernel("slic_distribution");
        slic_draw_map = context->getKernel("slic_draw_map");
        
        slic_rgb_to_cielab = context->getKernel("slic_rgb_to_cielab");
        slic_cielab_to_rgb = context->getKernel("slic_cielab_to_rgb");
        
        slic_normalise = context->getKernel("slic_normalise");
        slic_saliency = context->getKernel("slic_saliency");
        
        map_width = (_width - (_width % step))/step;
        map_height = (_height - (_height % step))/step;
        
        if ((_width - (map_width * step)) < step/2) {
            --map_width;
        }
        if ((_height - (map_height * step)) < step/2) {
            --map_height;
        }
        
        inputImage = context->floatImage(_width, _height);;
        
        clusters = context->buffer(_width*_height*sizeof(cl_int));
        distances = context->buffer(_width*_height*sizeof(cl_float));
        centers_col = context->buffer(map_width*map_height*sizeof(cl_float3)); // 5 channels
        centers_loc = context->buffer(map_width*map_height*sizeof(cl_int2)); // 5 channels
        centers_counts = context->buffer(map_width*map_height*sizeof(cl_int)); // 5 channels
        
        centers_col_squared = context->buffer(map_width*map_height*sizeof(cl_float3)); // 5 channels
        centers_col_blur = context->buffer(map_width*map_height*sizeof(cl_float3)); // 5 channels
        centers_col_squared_blur = context->buffer(map_width*map_height*sizeof(cl_float3)); // 5 channels
        centers_uniqueness = context->buffer(map_width*map_height*sizeof(cl_float));
        centers_distribution = context->buffer(map_width*map_height*sizeof(cl_float));
        centers_saliency = context->buffer(map_width*map_height*sizeof(cl_float));
        
        centers_loc_blur = context->buffer(map_width*map_height*sizeof(cl_float2));
        centers_loc_squared = context->buffer(map_width*map_height*sizeof(cl_float2));
        
        
        get_clusters = (cl_int *)context->hostReadBuffer(_width*_height*sizeof(cl_int));
        get_distances = (cl_float *)context->hostReadBuffer(_width*_height*sizeof(cl_float));
        get_centers_col = (cl_float3 *)context->hostReadBuffer(map_width*map_height*sizeof(cl_float3));
        get_centers_loc = (cl_int2 *)context->hostReadBuffer(map_width*map_height*sizeof(cl_int2));
        get_centers_counts = (cl_int *)context->hostReadBuffer(map_width*map_height*sizeof(cl_int));
        get_image = (cl_float4 *)context->hostReadBuffer(_width*_height*sizeof(cl_float4));
        
        get_centers_loc_squared = (cl_float2 *)context->hostReadBuffer(map_width*map_height*sizeof(cl_float2));
        
        put_clusters = (cl_int *)context->hostWriteBuffer(_width*_height*sizeof(cl_int));
        put_distances = (cl_float *)context->hostWriteBuffer(_width*_height*sizeof(cl_float));
        put_centers_col = (cl_float3 *)context->hostWriteBuffer(map_width*map_height*sizeof(cl_float3));
        put_centers_loc = (cl_int2 *)context->hostWriteBuffer(map_width*map_height*sizeof(cl_int2));
        put_centers_counts = (cl_int *)context->hostWriteBuffer(map_width*map_height*sizeof(cl_int));
        
        put_centers_loc_blur = (cl_float2 *)context->hostWriteBuffer(map_width*map_height*sizeof(cl_float2));
        
        put_image = (cl_float4 *)context->hostWriteBuffer(_width*_height*sizeof(cl_float4));
        
        //luma->setArg(1, *lumaImage);
        //luma->setArg(2, weight);
        //luma->setArg(3, gamma);
        //luma->setArg(4, darken);
        //luma->setArg(5, gammacorrect);
        
        addItem(context->floatImage(_width, _height));
        
        
        
        slic_clear_clusters->setArg(0, *clusters);
        slic_clear_distances->setArg(0, *distances);
        
        //slic_init_centers_counts->setArg(0, *((cl::Image2D *)mem[0]));
        slic_init_centers_counts->setArg(1, *centers_col);
        slic_init_centers_counts->setArg(2, *centers_loc);
        slic_init_centers_counts->setArg(3, *centers_counts);
        slic_init_centers_counts->setArg(4, step);
        
        slic_clear_centers_counts->setArg(0, *centers_col);
        slic_clear_centers_counts->setArg(1, *centers_loc);
        slic_clear_centers_counts->setArg(2, *centers_counts);
        
        //slic_update_allocation->setArg(0, *((cl::Image2D *)mem[0]));
        slic_update_allocation->setArg(1, *clusters);
        slic_update_allocation->setArg(2, *distances);
        slic_update_allocation->setArg(3, *centers_col);
        slic_update_allocation->setArg(4, *centers_loc);
        slic_update_allocation->setArg(5, step);
        slic_update_allocation->setArg(6, weight);
        slic_update_allocation->setArg(7, map_width*map_height);
        
        slic_update_allocation_pod->setArg(0, *inputImage);
        slic_update_allocation_pod->setArg(1, *clusters);
        slic_update_allocation_pod->setArg(2, *distances);
        slic_update_allocation_pod->setArg(3, *centers_col);
        slic_update_allocation_pod->setArg(4, *centers_loc);
        slic_update_allocation_pod->setArg(5, step);
        slic_update_allocation_pod->setArg(6, weight);
        slic_update_allocation_pod->setArg(7, _width);
        slic_update_allocation_pod->setArg(8, _height);
        
        //slic_update_cluster_center_p1->setArg(0, *((cl::Image2D *)mem[0]));
        slic_update_cluster_center_p1->setArg(1, *clusters);
        slic_update_cluster_center_p1->setArg(2, *centers_col);
        slic_update_cluster_center_p1->setArg(3, *centers_loc);
        slic_update_cluster_center_p1->setArg(4, *centers_counts);
        
        slic_update_cluster_center_p2->setArg(0, *centers_col);
        slic_update_cluster_center_p2->setArg(1, *centers_loc);
        slic_update_cluster_center_p2->setArg(2, *centers_counts);
        
        slic_draw_cells->setArg(0, *clusters);
        slic_draw_cells->setArg(1, *centers_col);
        slic_draw_cells->setArg(2, _getImageMem(0));
        
        slic_draw_contours->setArg(0, *clusters);
        slic_draw_contours->setArg(1, _getImageMem(0));
        
        slic_draw_centers->setArg(0, *centers_loc);
        slic_draw_centers->setArg(1, _getImageMem(0));
        
        slic_init_centers_counts->setArg(0, *inputImage);
        slic_update_allocation->setArg(0, *inputImage);
        slic_update_cluster_center_p1->setArg(0, *inputImage);
        
        
        slic_square_center_col->setArg(0, *centers_col);
        slic_square_center_col->setArg(1, *centers_col_squared);
        
        slic_square_center_loc->setArg(0, *centers_loc);
        slic_square_center_loc->setArg(1, *centers_loc_blur);
        slic_square_center_loc->setArg(2, *centers_loc_squared);
        
        slic_uniqueness->setArg(0, *centers_col);
        slic_uniqueness->setArg(1, *centers_col_squared);
        slic_uniqueness->setArg(2, *centers_col_blur);
        slic_uniqueness->setArg(3, *centers_col_squared_blur);
        slic_uniqueness->setArg(4, *centers_uniqueness);
        
        slic_distribution->setArg(0, *centers_loc_blur);
        slic_distribution->setArg(1, *centers_loc_squared);
        slic_distribution->setArg(2, *centers_distribution);
        
        slic_draw_map->setArg(0, *clusters);
        slic_draw_map->setArg(2, _getImageMem(0));
        
        
        slic_rgb_to_cielab->setArg(1, *inputImage);
        
        //slic_convert_test->setArg(0, *inputImage);
        //slic_convert_test->setArg(1, *((cl::Image2D *)mem[0]));
        
        slic_cielab_to_rgb->setArg(0, _getImageMem(0));
        slic_cielab_to_rgb->setArg(1, _getImageMem(0));
        
        slic_saliency->setArg(0, *centers_uniqueness);
        slic_saliency->setArg(1, *centers_distribution);
        slic_saliency->setArg(2, *centers_saliency);
        
        queue = context->getQueue();
    }
    
    float slic_compute_dist(const cl_float3 center_col, const cl_int2 center_loc, cl_int2 pixel, cl_float4 colour, int step, int weight) {
        float dc = sqrt(powf(center_col.s[0] - colour.s[0], 2.0f) + powf(center_col.s[1] - colour.s[1], 2.0f) + powf(center_col.s[2] - colour.s[2], 2.0f));
        float ds = sqrt(powf(center_loc.s[0] - (float)pixel.s[0], 2.0f) + powf(center_loc.s[1] - (float)pixel.s[1], 2.0f));
        
        return sqrt(powf(dc / (float)weight, 2.0f) + powf(ds / (float)step, 2.0f));
        
        //double w = 1.0 / (pow(ns / nc, 2));
        //return sqrt(dc) + sqrt(ds * w);
    }
    
    void process() {
        _profile->start();
        
        
        _profile->inReadStart();
        auto input = buffer->outLock();
        if (input == nullptr) {
            release();
            return;
        }
        
        _profile->inReadStop();
        
        
        _profile->writeToGPUStart();
		cl::size_t<3> origin;
		cl::size_t<3> region;
		origin[0] = 0; origin[1] = 0; origin[2] = 0;
		region[0] = _width; region[1] = _height; region[2] = 1;
        
        cl::Event event;
        
        slic_rgb_to_cielab->setArg(0, input.get_image());
//        queue->enqueueCopyImage(*input, *inputImage, origin, origin, region, NULL, &event);
//        event.wait();
        
        queue->enqueueNDRangeKernel(*slic_rgb_to_cielab, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        buffer->outUnlock();
        
//        static std::once_flag initFlag;
//        std::call_once(initFlag, [&]() {
            queue->enqueueNDRangeKernel(*slic_clear_clusters, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            queue->enqueueNDRangeKernel(*slic_init_centers_counts, cl::NullRange, cl::NDRange(map_width, map_height), cl::NullRange, NULL, &event);
            event.wait();
//        });
        
        
        queue->enqueueReadImage(*inputImage, CL_TRUE, origin, region, 0, 0, get_image, NULL, &event);
        event.wait();
        
        // CLEAN UP
        memset(put_clusters, 0, _width*_height*sizeof(cl_int));
        memset(put_distances, 0, _width*_height*sizeof(cl_float));
        memset(put_centers_col, 0, map_width*map_height*sizeof(cl_float3));
        memset(put_centers_loc, 0, map_width*map_height*sizeof(cl_int2));
        memset(put_centers_counts, 0, map_width*map_height*sizeof(cl_int));
        memset(put_centers_loc_blur, 0, map_width*map_height*sizeof(cl_float2));
        // CLEAN UP
        
        _profile->writeToGPUStop();
        
        
        
        _profile->executionStart();
        for (int i = 0; i < 10; ++i) {
            
            queue->enqueueNDRangeKernel(*slic_clear_distances, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            
        /*    queue->enqueueNDRangeKernel(*slic_update_allocation, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();*/
            
            /*for (int center = 0; center < map_width*map_height; ++center) {
                
            // Only compare to pixels in a 2 x step by 2 x step region.
                for (int x = get_centers_loc[center].x - step; x < get_centers_loc[center].x + step; x++) {
                    for (int y = get_centers_loc[center].y - step; y < get_centers_loc[center].y + step; y++) {
                        
                        if (x >= 0 && x < _width && y >= 0 && y < _height) {
                            int num = x+y*_width;
                            cl_float4 colour = get_image[num]; //CvScalar colour = cvGet2D(image, l, k);
                            cl_int2 coord;
                            coord.x = x;
                            coord.y = y;
                            float d = slic_compute_dist(get_centers_col[center], get_centers_loc[center], coord, colour, step, weight);
                            
                            // Update cluster allocation if the cluster minimizes the distance.
                            if (d < put_distances[num]) {
                                put_distances[num] = d;
                                put_clusters[num] = center;
                            }
                        }
                    }
                }
            }*/
            
            for (int x = -step; x < step; ++x) {
                for (int y = -step; y < step; ++y) {
                    
                    slic_update_allocation_pod->setArg(9, x);
                    slic_update_allocation_pod->setArg(10, y);
                    
                    queue->enqueueNDRangeKernel(*slic_update_allocation_pod, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
                    event.wait();
                    
                    
                }
            }
            
            /*queue->enqueueWriteBuffer(*distances, CL_TRUE, 0, _width*_height*sizeof(cl_float), put_distances, NULL, &event);
            event.wait();
            queue->enqueueWriteBuffer(*clusters, CL_TRUE, 0, _width*_height*sizeof(cl_int), put_clusters, NULL, &event);
            event.wait();*/
            
            queue->enqueueNDRangeKernel(*slic_clear_centers_counts, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
            
            queue->enqueueReadBuffer(*centers_col, CL_TRUE, 0, map_width*map_height*sizeof(cl_float3), get_centers_col, NULL, &event);
            event.wait();
            queue->enqueueReadBuffer(*centers_loc, CL_TRUE, 0, map_width*map_height*sizeof(cl_int2), get_centers_loc, NULL, &event);
            event.wait();
            queue->enqueueReadBuffer(*centers_counts, CL_TRUE, 0, map_width*map_height*sizeof(cl_int), get_centers_counts, NULL, &event);
            event.wait();
             
            queue->enqueueReadBuffer(*clusters, CL_TRUE, 0, _width*_height*sizeof(cl_int), get_clusters, NULL, &event);
            event.wait();
            queue->enqueueReadImage(*inputImage, CL_TRUE, origin, region, 0, 0, get_image, NULL, &event);
            event.wait();
            
            const int64_t imagesize = _width*_height;
            for (int64_t pixel = 0; pixel < imagesize; ++pixel) {
                
                int c_id = get_clusters[pixel];
                
                if (c_id != -1) {
                    const cl_float4& colour = get_image[pixel];  //cvGet2D(image, k, j); ALSO SUSPECT
                    const int64_t image_x = (pixel % _width);
                    const int64_t image_y = (pixel - image_x)/_width;
                    put_centers_col[c_id].s[0] += colour.s[0];
                    put_centers_col[c_id].s[1] += colour.s[1];
                    put_centers_col[c_id].s[2] += colour.s[2];
                    put_centers_loc[c_id].s[0] += image_x;
                    put_centers_loc[c_id].s[1] += image_y;
                    
                    put_centers_counts[c_id] += 1;
                }
            }
            
            queue->enqueueWriteBuffer(*centers_col, CL_TRUE, 0, map_width*map_height*sizeof(cl_float3), put_centers_col, NULL, &event);
            event.wait();
            queue->enqueueWriteBuffer(*centers_loc, CL_TRUE, 0, map_width*map_height*sizeof(cl_int2), put_centers_loc, NULL, &event);
            event.wait();
            queue->enqueueWriteBuffer(*centers_counts, CL_TRUE, 0, map_width*map_height*sizeof(cl_int), put_centers_counts, NULL, &event);
            event.wait();
            
            /*queue->enqueueNDRangeKernel(*slic_update_cluster_center_p1, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();*/
            
            queue->enqueueNDRangeKernel(*slic_update_cluster_center_p2, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
            
        }
        
        _profile->executionStop();
        
        inLock();
        
        
        _profile->writeStart();
        if (slicConfig.slicDrawUniqueness || slicConfig.slicDrawSaliency) {
            
            // UNIQUENESS
            
            queue->enqueueNDRangeKernel(*slic_square_center_col, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
            
            queue->enqueueReadBuffer(*centers_col, CL_TRUE, 0, map_width*map_height*sizeof(cl_float3), get_centers_col, NULL, &event);
            event.wait();
            queue->enqueueReadBuffer(*centers_loc, CL_TRUE, 0, map_width*map_height*sizeof(cl_int2), get_centers_loc, NULL, &event);
            event.wait();
            
            filter_col((float *)get_centers_col, (int *)get_centers_loc, (float *)put_centers_col);
            
            queue->enqueueWriteBuffer(*centers_col_blur, CL_TRUE, 0, map_width*map_height*sizeof(cl_float3), put_centers_col, NULL, &event);
            event.wait();
            
            queue->enqueueReadBuffer(*centers_col_squared, CL_TRUE, 0, map_width*map_height*sizeof(cl_float3), get_centers_col, NULL, &event);
            event.wait();
            
            filter_col((float *)get_centers_col, (int *)get_centers_loc, (float *)put_centers_col);
            
            queue->enqueueWriteBuffer(*centers_col_squared_blur, CL_TRUE, 0, map_width*map_height*sizeof(cl_float3), put_centers_col, NULL, &event);
            event.wait();
            
            queue->enqueueNDRangeKernel(*slic_uniqueness, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
            
            queue->enqueueReadBuffer(*centers_uniqueness, CL_TRUE, 0, map_width*map_height*sizeof(cl_float), get_distances, NULL, &event);
            event.wait();
            
            float big = 0;
            for (int po = 0; po < map_width*map_height; ++po) {
                const float chk = get_distances[po];
                if (chk > big) {
                    big = chk;
                }
            }
            
            slic_normalise->setArg(0, *centers_uniqueness);
            slic_normalise->setArg(1, big);
            
            queue->enqueueNDRangeKernel(*slic_normalise, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
            // UNIQUENESS

        }
        
        _profile->writeStop();
        
        _profile->readFromGPUStart();
        if (slicConfig.slicDrawDistribution || slicConfig.slicDrawSaliency) {
            
            // DISTRIBUTION
            
            queue->enqueueNDRangeKernel(*slic_square_center_loc, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
            
            queue->enqueueReadBuffer(*centers_col, CL_TRUE, 0, map_width*map_height*sizeof(cl_float3), get_centers_col, NULL, &event);
            event.wait();
            queue->enqueueReadBuffer(*centers_loc_blur, CL_TRUE, 0, map_width*map_height*sizeof(cl_float2), get_centers_loc_squared, NULL, &event);
            event.wait();
            
            filter_loc((float *)get_centers_loc_squared, (float *)get_centers_col, (float *)put_centers_loc_blur);
            
            queue->enqueueWriteBuffer(*centers_loc_blur, CL_TRUE, 0, map_width*map_height*sizeof(cl_float2), put_centers_loc_blur, NULL, &event);
            event.wait();
            
            queue->enqueueReadBuffer(*centers_loc_squared, CL_TRUE, 0, map_width*map_height*sizeof(cl_float2), get_centers_loc_squared, NULL, &event);
            event.wait();
            
            filter_loc((float *)get_centers_loc_squared, (float *)get_centers_col, (float *)put_centers_loc_blur);
            
            queue->enqueueWriteBuffer(*centers_loc_squared, CL_TRUE, 0, map_width*map_height*sizeof(cl_float2), put_centers_loc_blur, NULL, &event);
            event.wait();
            
            queue->enqueueNDRangeKernel(*slic_distribution, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
            
            queue->enqueueReadBuffer(*centers_distribution, CL_TRUE, 0, map_width*map_height*sizeof(cl_float), get_distances, NULL, &event);
            event.wait();
            
            float big = 0;
            for (int po = 0; po < map_width*map_height; ++po) {
                const float chk = get_distances[po];
                if (chk > big) {
                    big = chk;
                }
            }
            
            slic_normalise->setArg(0, *centers_distribution);
            slic_normalise->setArg(1, big);
            
            queue->enqueueNDRangeKernel(*slic_normalise, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
            
            // DISTRIBUTION
        }
        _profile->readFromGPUStop();
        
        
        _profile->waitStart();
        
        // COLOURING
        if (slicConfig.slicFillCells) {
            queue->enqueueNDRangeKernel(*slic_draw_cells, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
        }
        
        if (slicConfig.slicDrawCenters) {
            queue->enqueueNDRangeKernel(*slic_draw_centers, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
        }
        
        if (slicConfig.slicDrawBorders) {
            queue->enqueueNDRangeKernel(*slic_draw_contours, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
        }
        
        // CIELAB CUT
        
        queue->enqueueNDRangeKernel(*slic_cielab_to_rgb, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        if (slicConfig.slicDrawUniqueness) {
            slic_draw_map->setArg(1, *centers_uniqueness);
            slic_draw_map->setArg(3, 0);
            queue->enqueueNDRangeKernel(*slic_draw_map, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
        }
        
        if (slicConfig.slicDrawDistribution) {
            slic_draw_map->setArg(1, *centers_distribution);
            slic_draw_map->setArg(3, 1);
            queue->enqueueNDRangeKernel(*slic_draw_map, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
        }
        
        if (slicConfig.slicDrawSaliency) {
            queue->enqueueNDRangeKernel(*slic_saliency, cl::NullRange, cl::NDRange(map_width*map_height, 1), cl::NullRange, NULL, &event);
            event.wait();
            
            slic_draw_map->setArg(1, *centers_saliency);
            slic_draw_map->setArg(3, 0);
            queue->enqueueNDRangeKernel(*slic_draw_map, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            queue->enqueueReadImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, get_image, NULL, &event);
            event.wait();
            
            filter_im((float *)get_image, (float *)put_image);
            
 //           memcpy(put_image, get_image, sizeof(cl_float4)*_width*_height);
            queue->enqueueWriteImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, put_image, NULL, &event);
            event.wait();
        }
        
        // COLOURING
        
        inUnlock();
        
        _profile->waitStop();
        
        _profile->stop();
        _profile->report();
    }
    
    void filter_col(float * _centers_col, int * _centers_loc, float * p_centers_col) {
        
        PermutohedralLattice lattice(2, 4, map_width*map_height);
        
        float col[4];
        col[3] = 1; // homogeneous coordinate
        
        float * imPtr = (float *)_centers_col;
        int * refPtr = (int *)_centers_loc;
        for (int yl = 0; yl < map_height; ++yl) {
            for (int xl = 0; xl < map_width; ++xl) {
                
                for (int c = 0; c < 3; ++c) {
                    col[c] = *imPtr;
                    ++imPtr;
                }
                ++imPtr; // FORTH COLM
                
                float pos[2];
                pos[0] = 0.125 * (float)*refPtr;
                ++refPtr;
                pos[1] = 0.125 * (float)*refPtr;
                ++refPtr;
                
                lattice.splat(&pos[0], col);
                
            }
        }
        
        lattice.blur();
        
        lattice.beginSlice();
        
        float * outPtr = (float *)p_centers_col;
        
        for (int yl = 0; yl < map_height; ++yl) {
            for (int xl = 0; xl < map_width; ++xl) {
                lattice.slice(col);
                float scale = 1.0f/col[3];
                for (int c = 0; c < 4; ++c) {
                    *outPtr = col[c]*scale;
                    ++outPtr;
                }
            }
        }
    }
    
    
    void filter_loc(float * _centers_loc, float * _centers_col, float * p_centers_loc) {
        
        PermutohedralLattice lattice(3, 3, map_width*map_height);
        
        float col[3];
        
        col[2] = 1; // homogeneous coordinate
        
        float * imPtr = (float *)_centers_loc;
        float * refPtr = (float *)_centers_col;
        for (int yl = 0; yl < map_height; ++yl) {
            for (int xl = 0; xl < map_width; ++xl) {
                
                for (int c = 0; c < 2; ++c) {
                    col[c] = *imPtr;
                    ++imPtr;
                }
                //++imPtr; // FORTH COLM
                
                float pos[3];
                const float a = (float)((int)*refPtr);
                pos[0] = 20.0/255 * a;
                ++refPtr;
                const float b = (float)((int)*refPtr);
                pos[1] = 20.0/255 * b;
                ++refPtr;
                const float c = (float)((int)*refPtr);
                pos[2] = 20.0/255 * c;
                ++refPtr;++refPtr;
                
                lattice.splat(&pos[0], col);
                
            }
        }
        
        lattice.blur();
        
        lattice.beginSlice();
        
        float * outPtr = (float *)p_centers_loc;
        
        for (int yl = 0; yl < map_height; ++yl) {
            for (int xl = 0; xl < map_width; ++xl) {
                lattice.slice(col);
                float scale = 1.0f/col[2];
                for (int c = 0; c < 2; ++c) {
                    *outPtr = col[c]*scale;
                    ++outPtr;
                }
            }
        }
    }
    
    
    void filter_im(float * image, float * p_image) {
        
        PermutohedralLattice lattice(5, 4, _width*_height);
        
        float col[4];
        col[3] = 1; // homogeneous coordinate
        
        for (int yl = 0; yl < _height; ++yl) {
            for (int xl = 0; xl < _width; ++xl) {
                
                for (int c = 0; c < 3; ++c) {
                    col[c] = image[(yl*_width + xl)*4 + c];
                }
                
                float pos[5];
                pos[0] = 0.06 * (float)xl;
                pos[1] = 0.06 * (float)yl;
                pos[2] = 0.06 * 1.0/255 * image[(yl*_width + xl)*4];
                pos[2] = 0.06 * 1.0/255 * image[(yl*_width + xl)*4 + 1];
                pos[2] = 0.06 * 1.0/255 * image[(yl*_width + xl)*4 + 2];
                
                lattice.splat(&pos[0], col);
                
            }
        }
        
        lattice.blur();
        
        lattice.beginSlice();

        for (int yl = 0; yl < _height; ++yl) {
            for (int xl = 0; xl < _width; ++xl) {
                lattice.slice(col);
                float scale = 1.0f/col[3];
                for (int c = 0; c < 3; ++c) {
                    p_image[(yl*_width + xl)*4 + c] = col[c]*scale;
                }
                p_image[(yl*_width + xl)*4+3] = 1.0f;
            }
        }
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    
    cl::Kernel * slic_clear_clusters = nullptr;
    cl::Kernel * slic_clear_distances = nullptr;
    cl::Kernel * slic_init_centers_counts = nullptr;
    cl::Kernel * slic_clear_centers_counts = nullptr;
    cl::Kernel * slic_update_allocation = nullptr;
    cl::Kernel * slic_update_allocation_pod = nullptr;
    cl::Kernel * slic_update_cluster_center_p1 = nullptr;
    cl::Kernel * slic_update_cluster_center_p2 = nullptr;
    cl::Kernel * slic_draw_contours = nullptr;
    cl::Kernel * slic_draw_centers = nullptr;
    cl::Kernel * slic_draw_cells = nullptr;
    
    cl::Kernel * slic_square_center_col = nullptr;
    cl::Kernel * slic_square_center_loc = nullptr;
    cl::Kernel * slic_uniqueness = nullptr;
    cl::Kernel * slic_distribution = nullptr;
    
    cl::Kernel * slic_draw_map = nullptr;
    cl::Kernel * slic_draw_distribution = nullptr;
    
    cl::Kernel * slic_rgb_to_cielab = nullptr;
    cl::Kernel * slic_cielab_to_rgb = nullptr;
    
    cl::Kernel * slic_normalise = nullptr;
    
    cl::Kernel * slic_saliency = nullptr;
    
    cl::Image2D * inputImage = nullptr;
    
    
    cl_int * get_clusters = nullptr;
    cl_float * get_distances = nullptr;
    cl_float3 * get_centers_col = nullptr;
    cl_int2 * get_centers_loc = nullptr;
    cl_float4 * get_image = nullptr;
    cl_int * get_centers_counts = nullptr;
    
    cl_float2 * get_centers_loc_squared = nullptr;
    
    cl_int * put_clusters = nullptr;
    cl_float * put_distances = nullptr;
    cl_float3 * put_centers_col = nullptr;
    cl_int2 * put_centers_loc = nullptr;
    cl_int * put_centers_counts = nullptr;
    
    cl_float2 * put_centers_loc_blur = nullptr;
    
    cl_float4 * put_image = nullptr;
    
    
    cl::Buffer * clusters = nullptr;
    cl::Buffer * distances = nullptr;
    cl::Buffer * centers_col = nullptr; // 5 channels
    cl::Buffer * centers_loc = nullptr; // 5 channels
    cl::Buffer * centers_counts = nullptr; // 5 channels
    
    cl::Buffer * centers_col_squared = nullptr;
    
    cl::Buffer * centers_loc_squared = nullptr;
    cl::Buffer * centers_loc_blur = nullptr;
    
    cl::Buffer * centers_col_blur = nullptr;
    cl::Buffer * centers_col_squared_blur = nullptr;
    cl::Buffer * centers_uniqueness = nullptr;
    cl::Buffer * centers_distribution = nullptr;
    cl::Buffer * centers_saliency = nullptr;
    
    float weight = 0;
    int step = 0;
    
    unsigned int map_width = 0;
    unsigned int map_height = 0;
    
    mush::registerContainer<mush::imageBuffer> buffer;
    
    Profile * _profile = nullptr;
    
    mush::config::slicConfigStruct slicConfig;
};


#endif
