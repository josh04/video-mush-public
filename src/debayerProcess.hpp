//
//  debayerProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 04/08/2015.
//
//

#ifndef video_mush_debayerProcess_hpp
#define video_mush_debayerProcess_hpp

#include <sstream>
#include "azure/Event.hpp"
#include "azure/Eventable.hpp"
#include "azure/Events.hpp"
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>

typedef uint16_t ushort;

namespace mush {
    class debayerProcess : public mush::imageProcess, public azure::Eventable {
    public:
		debayerProcess() : mush::imageProcess(), _point({ 0, 0, 0, 0 }), image((ushort (*)[4]) calloc (_height, _width*sizeof(uint16_t)*4*4)) {
            
        }
        
        ~debayerProcess() {
            free(image);
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
            assert(buffers.size() == 1);
            
            debayer_green = context->getKernel("interpolate_green");
            debayer_red = context->getKernel("interpolate_red");
            debayer_blue = context->getKernel("interpolate_blue");
            transfer_function = context->getKernel("copyImage");
            
            vng_border = context->getKernel("vng_border_interpolate");
            vng_linear = context->getKernel("vng_lin_interpolate");
            vng_vng = context->getKernel("vng_interpolate");
            vng_green = context->getKernel("vng_green_equilibrate");
            
            buffer = castToImage(buffers.begin()[0]);
            
            buffer->getParams(_width, _height, _size);
            
            auto w_m =_width + 16 - _width % 16;
            auto h_m = _height + 16 - _height % 16;
            
            _temp = context->floatImage(w_m, h_m);
            _red = context->floatImage(w_m, h_m);
            _green = context->floatImage(w_m, h_m);
            
            _bit16 = context->int16bitImage(w_m, h_m);
            
            addItem(context->floatImage(w_m, h_m));
            
            
            queue = context->getQueue();
            
            
            int lookup[16][16][32];
            
            // build interpolation lookup table which for a given offset in the sensor
            // lists neighboring pixels from which to interpolate:
            // NUM_PIXELS                 # of neighboring pixels to read
            // for (1..NUM_PIXELS):
            //   OFFSET                   # in bytes from current pixel
            //   WEIGHT                   # how much weight to give this neighbor
            //   COLOR                    # sensor color
            // # weights of adjoining pixels not of this pixel's color
            // COLORA TOT_WEIGHT
            // COLORB TOT_WEIGHT
            // COLORPIX                   # color of center pixel
            int colors = 4;
            const unsigned int filters = 0x01010101u * 0x94u;
            const int size = (filters == 9) ? 6 : 16;
            
            constexpr uint32_t filters4 = filters | 0x0c0c0c0cu;
            /*if(filters == 9u) {
                filters4 = filters;
            } else if ((filters & 3) == 1) {
                filters4 = filters | 0x03030303u;
            } else {
                filters4 = filters | 0x0c0c0c0cu;
            }*/
            
            struct _roi {
                int x;
                int y;
                int width;
            } roi_{0,0, (int)_width};
            auto roi_in = &roi_;
            
            for(int row = 0; row < size; row++) {
                for(int col = 0; col < size; col++)
                {
                    int *ip = lookup[row][col] + 1;
                    int sum[4] = { 0, 0, 0, 0 };
                    const int f = fcol(row + roi_in->y, col + roi_in->x, filters4/*, xtrans*/);
                    // make list of adjoining pixel offsets by weight & color
                    for(int y = -1; y <= 1; y++) {
                        for(int x = -1; x <= 1; x++)
                        {
                            int weight = 1 << ((y == 0) + (x == 0));
                            const int color = fcol(row + y + roi_in->y, col + x + roi_in->x, filters4/*, xtrans*/);
                            if(color == f) continue;
                            //*ip++ = (roi_in->width * y + x);
                            
                            *ip++ = (y << 16) | (x & 0xffffu);
                            *ip++ = weight;
                            *ip++ = color;
                            sum[color] += weight;
                                                        
                        }
                    }
                    lookup[row][col][0] = (ip - lookup[row][col]) / 3; /* # of neighboring pixels found */
                    for(int c = 0; c < colors; c++) {
                        if(c != f) {
                            *ip++ = c;
                            *ip++ = sum[c];
                        }
                    }
                    *ip = f;
                }
            }
            
            for (int i = 0; i < 32; ++i) {
                //std::cout << lookup[i][0][0] <<std::endl;
            }
            
            cl::Event event;
            
            size_t lookup_size = sizeof(int) * 16 * 16 * 32;
            _lookup = context->buffer(lookup_size);
            
            queue->enqueueWriteBuffer(*_lookup, CL_TRUE, 0, lookup_size, lookup, NULL, &event);
            event.wait();
            
            
            
            constexpr int prow = (filters4 == 9u) ? 6 : 8;
            constexpr int pcol = (filters4 == 9u) ? 6 : 2;
            
            // Precalculate for VNG
            static const signed char terms[] = {
                -2,-2,+0,-1,0,0x01, -2,-2,+0,+0,1,0x01, -2,-1,-1,+0,0,0x01,
                -2,-1,+0,-1,0,0x02, -2,-1,+0,+0,0,0x03, -2,-1,+0,+1,1,0x01,
                -2,+0,+0,-1,0,0x06, -2,+0,+0,+0,1,0x02, -2,+0,+0,+1,0,0x03,
                -2,+1,-1,+0,0,0x04, -2,+1,+0,-1,1,0x04, -2,+1,+0,+0,0,0x06,
                -2,+1,+0,+1,0,0x02, -2,+2,+0,+0,1,0x04, -2,+2,+0,+1,0,0x04,
                -1,-2,-1,+0,0,static_cast<signed char>(0x80), -1,-2,+0,-1,0,0x01, -1,-2,+1,-1,0,0x01,
                -1,-2,+1,+0,1,0x01, -1,-1,-1,+1,0,static_cast<signed char>(0x88), -1,-1,+1,-2,0,0x40,
                -1,-1,+1,-1,0,0x22, -1,-1,+1,+0,0,0x33, -1,-1,+1,+1,1,0x11,
                -1,+0,-1,+2,0,0x08, -1,+0,+0,-1,0,0x44, -1,+0,+0,+1,0,0x11,
                -1,+0,+1,-2,1,0x40, -1,+0,+1,-1,0,0x66, -1,+0,+1,+0,1,0x22,
                -1,+0,+1,+1,0,0x33, -1,+0,+1,+2,1,0x10, -1,+1,+1,-1,1,0x44,
                -1,+1,+1,+0,0,0x66, -1,+1,+1,+1,0,0x22, -1,+1,+1,+2,0,0x10,
                -1,+2,+0,+1,0,0x04, -1,+2,+1,+0,1,0x04, -1,+2,+1,+1,0,0x04,
                +0,-2,+0,+0,1,static_cast<signed char>(0x80), +0,-1,+0,+1,1,static_cast<signed char>(0x88), +0,-1,+1,-2,0,0x40,
                +0,-1,+1,+0,0,0x11, +0,-1,+2,-2,0,0x40, +0,-1,+2,-1,0,0x20,
                +0,-1,+2,+0,0,0x30, +0,-1,+2,+1,1,0x10, +0,+0,+0,+2,1,0x08,
                +0,+0,+2,-2,1,0x40, +0,+0,+2,-1,0,0x60, +0,+0,+2,+0,1,0x20,
                +0,+0,+2,+1,0,0x30, +0,+0,+2,+2,1,0x10, +0,+1,+1,+0,0,0x44,
                +0,+1,+1,+2,0,0x10, +0,+1,+2,-1,1,0x40, +0,+1,+2,+0,0,0x60,
                +0,+1,+2,+1,0,0x20, +0,+1,+2,+2,0,0x10, +1,-2,+1,+0,0,static_cast<signed char>(0x80),
                +1,-1,+1,+1,0,static_cast<signed char>(0x88), +1,+0,+1,+2,0,0x08, +1,+0,+2,-1,0,0x40,
                +1,+0,+2,+1,0,0x10
            };
            static const signed char chood[]
            = { -1, -1, -1, 0, -1, +1, 0, +1, +1, +1, +1, 0, +1, -1, 0, -1 };
            int ips[prow * pcol * 352];
            int * ip = ips;
            int code[16][16];
            
            for(int row = 0; row < prow; row++) {
                for(int col = 0; col < pcol; col++) {
                    code[row][col] = ip - ips;
                    const signed char *cp = terms;
                    for(int t = 0; t < 64; t++) {
                        int y1 = *cp++, x1 = *cp++;
                        int y2 = *cp++, x2 = *cp++;
                        int weight = *cp++;
                        int grads = *cp++;
                        int color = fcol(row + y1, col + x1, filters4/*, xtrans*/);
                        if(fcol(row + y2, col + x2, filters4/*, xtrans*/) != color) continue;
                        int diag
                        = (fcol(row, col + 1, filters4/*, xtrans*/) == color && fcol(row + 1, col, filters4/*, xtrans*/) == color)
                        ? 2
                        : 1;
                        if(abs(y1 - y2) == diag && abs(x1 - x2) == diag) continue;
                        *ip++ = (y1 << 16) | (x1 & 0xffffu);
                        *ip++ = (y2 << 16) | (x2 & 0xffffu);
                        *ip++ = (color << 16) | (weight & 0xffffu);
                        for(int g = 0; g < 8; g++)
                            if(grads & 1 << g) *ip++ = g;
                        *ip++ = -1;
                    }
                    *ip++ = INT_MAX;
                    cp = chood;
                    for(int g = 0; g < 8; g++) {
                        int y = *cp++, x = *cp++;
                        *ip++ = (y << 16) | (x & 0xffffu);
                        int color = fcol(row, col, filters4/*, xtrans*/);
                        if(fcol(row + y, col + x, filters4/*, xtrans*/) != color
                           && fcol(row + y * 2, col + x * 2, filters4/*, xtrans*/) == color) {
                            *ip++ = (2*y << 16) | (2*x & 0xffffu);
                            *ip++ = color;
                        } else {
                            *ip++ = 0;
                            *ip++ = 0;
                        }
                    }
                }
            }
            
            size_t ips_size = sizeof(int) * prow * pcol * 352;
            _ips = context->buffer(ips_size);
            
            queue->enqueueWriteBuffer(*_ips, CL_TRUE, 0, ips_size, ips, NULL, &event);
            event.wait();
            
            size_t code_size = sizeof(int) * 16 * 16;
            _code = context->buffer(code_size);
            
            queue->enqueueWriteBuffer(*_code, CL_TRUE, 0, code_size, code, NULL, &event);
            event.wait();
            
            
        }
        
        
        
        
        void process() {
            inLock();
            auto input = buffer->outLock();
            if (input == nullptr) {
                release();
                return;
            }
            
            cl::Event event;
            /*
             displace->setArg(0, input.get_image());
             
             queue->enqueueNDRangeKernel(*displace, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
             event.wait();
             */
#define VNG_CL
#ifdef VNG
            transfer_function->setArg(0, input.get_image());
            transfer_function->setArg(1, *_bit16);
            
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            cl::size_t<3> origin, region;
            origin[0] = 0; origin[1] = 0; origin[2] = 0;
            region[0] = _width; region[1] = _height; region[2] = 1;
            
            queue->enqueueReadImage(*(_bit16), CL_TRUE, origin, region, 0, 0, image, NULL, &event);
            event.wait();
            /*
            for (int i = 0; i < _height; ++i) {
                for (int j = 0; j < _width; ++j) {
                    unsigned short a = 0, b = 0, c = 0, d = 0;
                    auto& four = image[i * _width + j];
                    if (j % 2 == 0 && i % 2 == 0) {
                        a = image[i * _width + j][0] * 2.0f;
                    } else if (j % 2 == 1 && i % 2 == 1) {
                        c = image[i * _width + j][2] * 1.5f;
                    } else  {
                        b = image[i * _width + j][1];
                    }
                image[i * _width + j][0] = a;
                image[i * _width + j][1] = b;
                image[i * _width + j][2] = c;
                image[i * _width + j][3] = 0;
                    //image[i][3] = 65535;
                }
            }
            */
            vng_interpolate(_width, _height);
            
            uint16_t m = 0;
            
            for (int i = 0; i < _width * _height; ++i) {
                
                    //m = max(image[i][1], m);
                
            //    for (int j = 0; j < 4; ++j) {
                    //image[i][1] = (image[i][1] + image[i][3]);
                    //image[i][3] = 65535;
            //    }
            }
            
            queue->enqueueWriteImage(*(_bit16), CL_TRUE, origin, region, 0, 0, image, NULL, &event);
            event.wait();
            
            
            transfer_function->setArg(0, *(_bit16));
            transfer_function->setArg(1, _getImageMem(0));
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
  
#elif defined(VNG_CL)
            
            const unsigned int filters = 0x01010101u * 0x94u;
            
            uint32_t filters4;
            if(filters == 9u) {
                filters4 = filters;
            } else if ((filters & 3) == 1) {
                filters4 = filters | 0x03030303u;
            } else {
                filters4 = filters | 0x0c0c0c0cu;
            }
            
            /*
            transfer_function->setArg(0, input.get_image());
            transfer_function->setArg(1, *_bit16);
            
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            
            cl::size_t<3> origin, region;
            origin[0] = 0; origin[1] = 0; origin[2] = 0;
            region[0] = _width; region[1] = _height; region[2] = 1;
            
            queue->enqueueReadImage(*(_bit16), CL_TRUE, origin, region, 0, 0, image, NULL, &event);
            event.wait();
            
            for (int i = 0; i < _height; ++i) {
                for (int j = 0; j < _width; ++j) {
                    unsigned short a = 0, b = 0, c = 0, d = 0;
                    auto& four = image[i * _width + j];
                    if (j % 2 == 0 && i % 2 == 0) {
                        a = image[i * _width + j][0];
                    } else if (j % 2 == 1 && i % 2 == 1) {
                        c = image[i * _width + j][2];
                    } else  {
                        b = image[i * _width + j][1];
                    }
                    image[i * _width + j][0] = a;
                    image[i * _width + j][1] = b;
                    image[i * _width + j][2] = c;
                    image[i * _width + j][3] = 0;
                    //image[i][3] = 65535;
                }
            }
            
            queue->enqueueWriteImage(*(_bit16), CL_TRUE, origin, region, 0, 0, image, NULL, &event);
            event.wait();
            
            
            transfer_function->setArg(0, *(_bit16));
            transfer_function->setArg(1, input.get_image());
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            */
            
            
            /*
             kernel void
             vng_lin_interpolate(read_only image2d_t in, write_only image2d_t out, const int width, const int height,
             const unsigned int filters, global const int (*const lookup)[16][32],
             local float *buffer)
             */
            
            int blocksizex = 16;
            int blocksizey = 16;
            
            auto w_m =_width + blocksizex - _width % blocksizex;
            auto h_m = _height + blocksizey - _height % blocksizey;
            
            transfer_function->setArg(0, input.get_image());
            transfer_function->setArg(1, *_temp);
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            vng_linear->setArg(0, input.get_image());
            vng_linear->setArg(1, *_temp);
            
            vng_linear->setArg(2, _width);
            vng_linear->setArg(3, _height);
            vng_linear->setArg(4, filters4);
            vng_linear->setArg(5, *_lookup);
            vng_linear->setArg(6, cl::Local((blocksizex + 2) * (blocksizey + 2) * sizeof(float)));
            
            
            queue->enqueueNDRangeKernel(*vng_linear, cl::NullRange, cl::NDRange(w_m, h_m), cl::NDRange(blocksizex, blocksizey), NULL, &event);
            event.wait();
            
            
            /*
             kernel void
             vng_interpolate(read_only image2d_t in, write_only image2d_t out, const int width, const int height,
             const int rin_x, const int rin_y, const unsigned int filters, const float4 processed_maximum,
             global const unsigned char (*const xtrans)[6], global const int (*const ips),
             global const int (*const code)[16], local float *buffer)
             */
            
            /*
             kernel void
             vng_border_interpolate(read_only image2d_t in, write_only image2d_t out, const int width, const int height,
             const int r_x, const int r_y, const unsigned int filters, global const unsigned char (*const xtrans)[6])
             */
            
            transfer_function->setArg(0, *_temp);
            transfer_function->setArg(1, *_green);
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            vng_vng->setArg(0, *_temp);
            vng_vng->setArg(1, *_green);
            
            vng_vng->setArg(2, _width);
            vng_vng->setArg(3, _height);
            vng_vng->setArg(4, 0);
            vng_vng->setArg(5, 0);
            vng_vng->setArg(6, filters4);
            vng_vng->setArg(7, cl_float4{100.0f, 100.0f, 100.0f, 100.0f});
            vng_vng->setArg(8, *_ips);
            vng_vng->setArg(9, *_code);
            vng_vng->setArg(10, cl::Local((blocksizex + 4) * (blocksizey + 4) * 4 * sizeof(float)));
            
            queue->enqueueNDRangeKernel(*vng_vng, cl::NullRange, cl::NDRange(w_m, h_m), cl::NDRange(blocksizex, blocksizey), NULL, &event);
            event.wait();
            
            transfer_function->setArg(0, *_green);
            transfer_function->setArg(1, *_red);
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            vng_border->setArg(0, *_green);
            vng_border->setArg(1, *_red);
            
            vng_border->setArg(2, _width);
            vng_border->setArg(3, _height);
            vng_border->setArg(4, 0);
            vng_border->setArg(5, 0);
            vng_border->setArg(6, filters4);
            
            queue->enqueueNDRangeKernel(*vng_border, cl::NullRange, cl::NDRange(w_m, h_m), cl::NullRange, NULL, &event);
            event.wait();
            
            
            vng_green->setArg(0, *_red);
            vng_green->setArg(1, _getImageMem(0));
            vng_green->setArg(2, _width);
            vng_green->setArg(3, _height);
            
            queue->enqueueNDRangeKernel(*vng_green, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            /*
            transfer_function->setArg(0, *(_getImageMem(0)));
            transfer_function->setArg(1, *_bit16);
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            transfer_function->setArg(0, *_bit16);
            transfer_function->setArg(1, _getImageMem(0));
            queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait()
             */
#else
            
            debayer_green->setArg(0, input.get_image());
            debayer_green->setArg(1, *_green);
            queue->enqueueNDRangeKernel(*debayer_green, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            debayer_red->setArg(0, input.get_image());
            debayer_red->setArg(1, *_green);
            debayer_red->setArg(2, *_red);
            queue->enqueueNDRangeKernel(*debayer_red, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
            
            debayer_blue->setArg(0, input.get_image());
            debayer_blue->setArg(1, *_red);
            debayer_blue->setArg(2, _getImageMem(0));
            queue->enqueueNDRangeKernel(*debayer_blue, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
            event.wait();
             
             transfer_function->setArg(0, *(_getImageMem(0)));
             transfer_function->setArg(1, *_bit16);
             queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
             event.wait();
             transfer_function->setArg(0, *_bit16);
             transfer_function->setArg(1, _getImageMem(0));
             queue->enqueueNDRangeKernel(*transfer_function, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
             event.wait();
            
#endif
            
            
            
            buffer->outUnlock();
            
            inUnlock();
        }
        
        bool event(std::shared_ptr<azure::Event> event) {
            if (event->isType("whitePointClick")) {
                
                float x = event->getAttribute<float>("x");
                float y = event->getAttribute<float>("y");
                
                cl::Event event;
                cl::size_t<3> origin, region;
                origin[0] = (int)x; origin[1] = (int)y; origin[2] = 0;
                region[0] = 1; region[1] = 1; region[2] = 1;
                
                queue->enqueueReadImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, &_point, NULL, &event);
                event.wait();
                _point.s0 = _point.s1 / _point.s0;
                _point.s2 = _point.s1 / _point.s2;
                _point.s1 = 1.0f;
                azure::Events::Push(std::unique_ptr<azure::Event>(new azure::Event("whitePointFromDebayer", 0)));
            }
            return false;
        }
        
        cl_float4 getWhitePoint() {
            return _point;
        }
        
        /*
         This algorithm is officially called:
         
         "Interpolation using a Threshold-based variable number of gradients"
         
         described in http://scien.stanford.edu/pages/labsite/1999/psych221/projects/99/tingchen/algodep/vargra.html
         
         I've extended the basic idea to work with non-Bayer filter arrays.
         Gradients are numbered clockwise from NW=0 to W=7.
         */
    
        
        /*
         
         
         All RGB cameras use one of these Bayer grids:
         
         0x16161616:	0x61616161:     0x49494949:     0x94949494:
         
         0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5	  0 1 2 3 4 5
         0 B G B G B G	0 G R G R G R	0 G B G B G B	0 R G R G R G
         1 G R G R G R	1 B G B G B G	1 R G R G R G	1 G B G B G B
         2 B G B G B G	2 G R G R G R	2 G B G B G B	2 R G R G R G
         3 G R G R G R	3 B G B G B G	3 R G R G R G	3 G B G B G B
         */
        
        
#define FC(row,col) (filters >> ((((row) << 1 & 14) + ((col) & 1)) << 1) & 3)
    
        int fcol (int row, int col)
        {
            unsigned int filters = 0x01010101u * 0x94u;
            //filters &= ~((filters & 0x55555555) << 1);
            return FC(row,col);
        }
        
        int fcol (int row, int col, int filters)
        {
            //unsigned int filters = 0x01010101u * 0x94u;
            //filters &= ~((filters & 0x55555555) << 1);
            return FC(row,col);
        }

        
        void border_interpolate (int border, int height, int width)
        {
            int colors = 3;
            unsigned row, col, y, x, f, sum[8];
            
            for (row=0; row < height; row++)
                for (col=0; col < width; col++) {
                    if (col==border && row >= border && row < height-border)
                        col = width-border;
                    memset (sum, 0, sizeof sum);
                    for (y=row-1; y != row+2; y++)
                        for (x=col-1; x != col+2; x++)
                            if (y < height && x < width) {
                                f = fcol(y,x);
                                sum[f] += image[y*width+x][f];
                                sum[f+4]++;
                            }
                    f = fcol(row,col);
                    for (int c = 0; c < colors; ++c) {
                        if (c != f && sum[c+4]) {
                            image[row*width+col][c] = sum[c] / sum[c+4];
                        }
                    }
                }
        }
        
        void lin_interpolate(unsigned int width, unsigned int height)
        {
            int colors = 3;
            bool verbose = true;
            int code[16][16][32], size=16, *ip, sum[4];
            int f, c, i, x, y, row, col, shift, color;
            ushort *pix;
            
            if (verbose) fprintf (stderr, "Bilinear interpolation...\n");
            //if (filters == 9) size = 6;
            border_interpolate(1, height, width);
            for (row=0; row < size; row++)
                for (col=0; col < size; col++) {
                    ip = code[row][col]+1;
                    f = fcol(row,col);
                    memset (sum, 0, sizeof sum);
                    for (y=-1; y <= 1; y++)
                        for (x=-1; x <= 1; x++) {
                            shift = (y==0) + (x==0);
                            color = fcol(row+y,col+x);
                            if (color == f) continue;
                            *ip++ = (width*y + x)*4 + color;
                            *ip++ = shift;
                            *ip++ = color;
                            sum[color] += 1 << shift;
                        }
                    code[row][col][0] = (ip - code[row][col]) / 3;
                    for (int c = 0; c < colors; ++c) {
                        if (c != f) {
                            *ip++ = c;
                            *ip++ = 256 / sum[c];
                        }
                    }
                }
            for (row=1; row < height-1; row++)
                for (col=1; col < width-1; col++) {
                    pix = image[row*width+col];
                    ip = code[row % size][col % size];
                    memset (sum, 0, sizeof sum);
                    for (i=*ip++; i--; ip+=3)
                        sum[ip[2]] += pix[ip[0]] << ip[1];
                    for (i=colors; --i; ip+=2)
                        pix[ip[0]] = sum[ip[0]] * ip[1] >> 8;
                }
        }

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define LIM(x,min,max) MAX(min,MIN(x,max))
#define ULIM(x,y,z) ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))
#define CLIP(x) LIM((int)(x),0,65535)
        
        void vng_interpolate(unsigned int width, unsigned int height)
        {
            
            
            
            
            int colors = 3;
            bool verbose = true;
            static const signed char *cp, terms[] = {
                -2,-2,+0,-1,0,0x01, -2,-2,+0,+0,1,0x01, -2,-1,-1,+0,0,0x01,
                -2,-1,+0,-1,0,0x02, -2,-1,+0,+0,0,0x03, -2,-1,+0,+1,1,0x01,
                -2,+0,+0,-1,0,0x06, -2,+0,+0,+0,1,0x02, -2,+0,+0,+1,0,0x03,
                -2,+1,-1,+0,0,0x04, -2,+1,+0,-1,1,0x04, -2,+1,+0,+0,0,0x06,
                -2,+1,+0,+1,0,0x02, -2,+2,+0,+0,1,0x04, -2,+2,+0,+1,0,0x04,
                -1,-2,-1,+0,0,static_cast<signed char>(0x80), -1,-2,+0,-1,0,0x01, -1,-2,+1,-1,0,0x01,
                -1,-2,+1,+0,1,0x01, -1,-1,-1,+1,0,static_cast<signed char>(0x88), -1,-1,+1,-2,0,0x40,
                -1,-1,+1,-1,0,0x22, -1,-1,+1,+0,0,0x33, -1,-1,+1,+1,1,0x11,
                -1,+0,-1,+2,0,0x08, -1,+0,+0,-1,0,0x44, -1,+0,+0,+1,0,0x11,
                -1,+0,+1,-2,1,0x40, -1,+0,+1,-1,0,0x66, -1,+0,+1,+0,1,0x22,
                -1,+0,+1,+1,0,0x33, -1,+0,+1,+2,1,0x10, -1,+1,+1,-1,1,0x44,
                -1,+1,+1,+0,0,0x66, -1,+1,+1,+1,0,0x22, -1,+1,+1,+2,0,0x10,
                -1,+2,+0,+1,0,0x04, -1,+2,+1,+0,1,0x04, -1,+2,+1,+1,0,0x04,
                +0,-2,+0,+0,1,static_cast<signed char>(0x80), +0,-1,+0,+1,1,static_cast<signed char>(0x88), +0,-1,+1,-2,0,0x40,
                +0,-1,+1,+0,0,0x11, +0,-1,+2,-2,0,0x40, +0,-1,+2,-1,0,0x20,
                +0,-1,+2,+0,0,0x30, +0,-1,+2,+1,1,0x10, +0,+0,+0,+2,1,0x08,
                +0,+0,+2,-2,1,0x40, +0,+0,+2,-1,0,0x60, +0,+0,+2,+0,1,0x20,
                +0,+0,+2,+1,0,0x30, +0,+0,+2,+2,1,0x10, +0,+1,+1,+0,0,0x44,
                +0,+1,+1,+2,0,0x10, +0,+1,+2,-1,1,0x40, +0,+1,+2,+0,0,0x60,
                +0,+1,+2,+1,0,0x20, +0,+1,+2,+2,0,0x10, +1,-2,+1,+0,0,static_cast<signed char>(0x80),
                +1,-1,+1,+1,0,static_cast<signed char>(0x88), +1,+0,+1,+2,0,0x08, +1,+0,+2,-1,0,0x40,
                +1,+0,+2,+1,0,0x10
            }, chood[] = { -1,-1, -1,0, -1,+1, 0,+1, +1,+1, +1,0, +1,-1, 0,-1 };
            ushort (*brow[5])[4], *pix;
            int prow=8, pcol=2, *ip, *code[16][16], gval[8], gmin, gmax, sum[4];
            int row, col, x, y, x1, x2, y1, y2, t, weight, grads, color, diag;
            int g, diff, thold, num, c;
            
            lin_interpolate(width, height);
            if (verbose) fprintf (stderr, "VNG interpolation...\n");
            
            //if (filters == 1) prow = pcol = 16;
            //if (filters == 9) prow = pcol =  6;
            ip = (int *) calloc (prow*pcol, 1280);
            //merror (ip, "vng_interpolate()");
            for (row=0; row < prow; row++)		/* Precalculate for VNG */
                for (col=0; col < pcol; col++) {
                    code[row][col] = ip;
                    for (cp=terms, t=0; t < 64; t++) {
                        y1 = *cp++;  x1 = *cp++;
                        y2 = *cp++;  x2 = *cp++;
                        weight = *cp++;
                        grads = *cp++;
                        color = fcol(row+y1,col+x1);
                        if (fcol(row+y2,col+x2) != color) continue;
                        diag = (fcol(row,col+1) == color && fcol(row+1,col) == color) ? 2:1;
                        if (abs(y1-y2) == diag && abs(x1-x2) == diag) continue;
                        *ip++ = (y1*width + x1)*4 + color;
                        *ip++ = (y2*width + x2)*4 + color;
                        *ip++ = weight;
                        for (g=0; g < 8; g++)
                            if (grads & 1<<g) *ip++ = g;
                        *ip++ = -1;
                    }
                    *ip++ = INT_MAX;
                    for (cp=chood, g=0; g < 8; g++) {
                        y = *cp++;  x = *cp++;
                        *ip++ = (y*width + x) * 4;
                        color = fcol(row,col);
                        if (fcol(row+y,col+x) != color && fcol(row+y*2,col+x*2) == color)
                            *ip++ = (y*width + x) * 8 + color;
                        else
                            *ip++ = 0;
                    }
                }
            brow[4] = (ushort (*)[4]) calloc (width*3, sizeof **brow);
            //merror (brow[4], "vng_interpolate()");
            for (row=0; row < 3; row++)
                brow[row] = brow[4] + row*width;
            for (row=2; row < height-2; row++) {		/* Do VNG interpolation */
                for (col=2; col < width-2; col++) {
                    pix = image[row*width+col];
                    ip = code[row % prow][col % pcol];
                    memset (gval, 0, sizeof gval);
                    while ((g = ip[0]) != INT_MAX) {		/* Calculate gradients */
                        diff = abs(pix[g] - pix[ip[1]]) << ip[2];
                        gval[ip[3]] += diff;
                        ip += 5;
                        if ((g = ip[-1]) == -1) continue;
                        gval[g] += diff;
                        while ((g = *ip++) != -1)
                            gval[g] += diff;
                    }
                    ip++;
                    gmin = gmax = gval[0];			/* Choose a threshold */
                    for (g=1; g < 8; g++) {
                        if (gmin > gval[g]) gmin = gval[g];
                        if (gmax < gval[g]) gmax = gval[g];
                    }
                    if (gmax == 0) {
                        memcpy (brow[2][col], pix, sizeof *image);
                        continue;
                    }
                    thold = gmin + (gmax >> 1);
                    memset (sum, 0, sizeof sum);
                    color = fcol(row,col);
                    for (num=g=0; g < 8; g++,ip+=2) {		/* Average the neighbors */
                        if (gval[g] <= thold) {
                            for (int c = 0; c < colors; ++c) {
                                if (c == color && ip[1]) {
                                    sum[c] += (pix[c] + pix[ip[1]]) >> 1;
                                } else {
                                    sum[c] += pix[ip[0] + c];
                                }
                            }
                            num++;
                        }
                    }
                    for (int c = 0; c < colors; ++c) {					/* Save to buffer */
                        t = pix[color];
                        if (c != color)
                            t += (sum[c] - sum[color]) / num;
                        brow[2][col][c] = CLIP(t);
                    }
                }
                if (row > 3)				/* Write buffer to image */
                    memcpy (image[(row-2)*width+2], brow[0]+2, (width-4)*sizeof *image);
                for (g=0; g < 4; g++)
                    brow[(g-1) & 3] = brow[g];
            }
            memcpy (image[(row-2)*width+2], brow[0]+2, (width-4)*sizeof *image);
            memcpy (image[(row-1)*width+2], brow[1]+2, (width-4)*sizeof *image);
            free (brow[4]);
            free (code[0][0]);
        }
        
    private:
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * debayer_green = nullptr;
        cl::Kernel * debayer_red = nullptr;
        cl::Kernel * debayer_blue = nullptr;
        cl::Kernel * transfer_function = nullptr;
        
        cl::Kernel * vng_border = nullptr;
        cl::Kernel * vng_linear = nullptr;
        cl::Kernel * vng_vng = nullptr;
        cl::Kernel * vng_green = nullptr;
    
        cl::Image2D * _green = nullptr, * _temp = nullptr, * _red = nullptr, * _bit16 = nullptr;
        
        
        cl_float4 _point;
        
        std::shared_ptr<mush::imageBuffer> buffer = nullptr;
        
        uint16_t (* image)[4];
        
        cl::Buffer * _lookup = nullptr, * _code = nullptr, * _ips = nullptr;
    };
    
}

#endif
