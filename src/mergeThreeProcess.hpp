//
//  mergeThreeProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_mergeThreeProcess_hpp
#define video_mush_mergeThreeProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>


class mergeThreeProcess : public mush::imageProcess {
public:
    mergeThreeProcess(float * isos) : mush::imageProcess(), isos(isos) {
        
    }
    
    ~mergeThreeProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 3);
        merge = context->getKernel("merge3");
        toGray = context->getKernel("toGray");
        average = context->getKernel("averages");
        
        //        merge->setArg(2, gamma);
        
        buffer = castToImage(buffers.begin()[0]);
        buffer2 = castToImage(buffers.begin()[1]);
        buffer3 = castToImage(buffers.begin()[2]);
        
        buffer->getParams(_width, _height, _size);
        
        queue = context->getQueue();
        
        isoBuffer = context->buffer(sizeof(cl_float) * 3, CL_MEM_READ_ONLY);
		queue->enqueueWriteBuffer(*isoBuffer, CL_TRUE, 0, sizeof(cl_float)*3, isos, NULL, NULL);
        
		averageTemp = context->floatImage(_width, _height);;
		averageTemp2 = context->floatImage(_width, _height);;
        
		pixData = (float *) context->hostReadBuffer(sizeof(float) * 40);
        
        addItem(context->floatImage(_width, _height));
    }
    
    void process() {
        cl::Event event;
        
        // alpha
        auto alphaCopy = buffer->outLock();
        if (alphaCopy == nullptr) {
            release();
            return;
        }
        
        // beta
        auto betaCopy = buffer2->outLock();
        if (betaCopy == nullptr) {
            release();
            return;
        }
        
        auto input = buffer3->outLock();
        if (input == nullptr) {
            release();
            return;
        }
        
        float alphaAverage[4];
        float betaAverage[4];
        float gammaAverage[4];
        {
            doAverage(alphaCopy.get_image(), alphaAverage);
            doAverage(betaCopy.get_image(), betaAverage);
            doAverage(input.get_image(), gammaAverage);
        }
        
        merge->setArg(0, _getImageMem(0));
        merge->setArg(4, *isoBuffer);
        
        if (alphaAverage[0] < betaAverage[0]) {
            if (gammaAverage[0] < alphaAverage[0]) {
                merge->setArg(1, input.get_image());
                merge->setArg(2, alphaCopy.get_image());
                merge->setArg(3, betaCopy.get_image());
            } else {
                if (gammaAverage[0] < betaAverage[0]) {
                    merge->setArg(1, alphaCopy.get_image());
                    merge->setArg(2, input.get_image());
                    merge->setArg(3, betaCopy.get_image());
                } else {
                    merge->setArg(1, alphaCopy.get_image());
                    merge->setArg(2, betaCopy.get_image());
                    merge->setArg(3, input.get_image());
                }
            }
        } else {
            if (gammaAverage[0] < betaAverage[0]) {
                merge->setArg(1, input.get_image());
                merge->setArg(2, betaCopy.get_image());
                merge->setArg(3, alphaCopy.get_image());
            } else {
                if (gammaAverage[0] < alphaAverage[0]) {
                    merge->setArg(1, betaCopy.get_image());
                    merge->setArg(2, input.get_image());
                    merge->setArg(3, alphaCopy.get_image());
                } else {
                    merge->setArg(1, betaCopy.get_image());
                    merge->setArg(2, alphaCopy.get_image());
                    merge->setArg(3, input.get_image());
                }
            }
        }
        
        inLock();
        queue->enqueueNDRangeKernel(*merge, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        buffer->outUnlock();
        buffer2->outUnlock();
        buffer3->outUnlock();
        inUnlock();
    }

	void setIsos(const float &iso1, const float &iso2, const float &iso3) {
		if (isoBuffer != nullptr) {
			isos[0] = iso1;
			isos[1] = iso2;
			isos[2] = iso3;
			queue->enqueueWriteBuffer(*isoBuffer, CL_TRUE, 0, sizeof(cl_float) * 3, &isos[0], NULL, NULL);
		}
	}
    
private:
    
	void doAverage(cl_mem averaged, float * output) {
		cl::Event event;
		cl::size_t<3> origin;
		cl::size_t<3> region;
		toGray->setArg(0, averaged);
		toGray->setArg(1, *averageTemp);
		origin[0] = 0; origin[1] = 0; origin[2] = 0;
		region[0] = _width; region[1] = _height; region[2] = 1;
		
		queue->enqueueNDRangeKernel(*toGray, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
		event.wait();
        
		int limit = 10;
		int scale = 4;
		
		cl::Image2D * tmp;
		unsigned int x = _width; unsigned int y = _height;
		
		
		float lmin = 1.0f, lmax = 1.0f, avg = 1.0f;
		// OpenCL
		while (x * y > limit) {
			average->setArg(0, *averageTemp);
			average->setArg(1, *averageTemp2);
			average->setArg(2, x);
            average->setArg(3, y);
            average->setArg(4, scale);
			
			x = ceil((double)x / (double)scale); if (x < 1) {x = 1;}
			y = ceil((double)y / (double)scale); if (y < 1) {y = 1;}
			queue->enqueueNDRangeKernel(*average, cl::NullRange, cl::NDRange(x, y), cl::NullRange, NULL, &event);
			event.wait();
			/*
			 queue->enqueueReadImage(*a, CL_TRUE, origin, region, 0, 0, pixData, NULL, &event);
			 event.wait();
			 */
			tmp = averageTemp; averageTemp = averageTemp2; averageTemp2 = tmp;
		}
		// CPU
		
		if (x * y > 1) {
			//		tmp = a; a = b; b = tmp;
			region[0] = x; region[1] = y;
			double color = 0.0;
			queue->enqueueReadImage(*averageTemp, CL_TRUE, origin, region, 0, 0, pixData, NULL, &event);
			event.wait();
			
			lmin = 1.0f; lmax = 0.0f; avg = 0.0f;
			
			
			for (unsigned int i = 0; i < x * y; ++i) {
				
				color += *(pixData + (i*4));
				
				lmax = (lmax > *(pixData + (i*4) + 1))
				? lmax : *(pixData + (i*4) + 1);
				
				lmin = (lmin < *(pixData + (i*4) + 2))
				? lmin : *(pixData + (i*4) + 2);
				
				avg += *(pixData + (i*4) + 3);
				
			}
			
			color = color / (double)(x * y);
			
			*(pixData) = color;
			*(pixData + 1) = lmax;
			*(pixData + 2) = lmin;
			*(pixData + 3) = avg;
			
		} else {
			region[0] = 1; region[1] = 1;
			queue->enqueueReadImage(*averageTemp, CL_TRUE, origin, region, 0, 0, pixData);
		}
        
		float h = *pixData;
		float ma = *(pixData + 1);
		float mi = *(pixData + 2);
		float av = *(pixData + 3) / (double) (_width * _height);
        
		output[0] = h;												// hmean
		output[1] = ma;								// max luminance
		output[2] = mi;								// min luminance
		output[3] = av;			// mean
	}
    
    
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * merge = nullptr;
    cl::Kernel * toGray = nullptr;
    cl::Kernel * average = nullptr;
    
    cl::Image2D * averageTemp = nullptr;
    cl::Image2D * averageTemp2 = nullptr;
    cl::Buffer * isoBuffer = nullptr;
    
	float * pixData = nullptr;
    float * isos = nullptr;
    
    float gamma = 0.0f;
    mush::registerContainer<mush::imageBuffer> buffer;
    mush::registerContainer<mush::imageBuffer> buffer2;
    mush::registerContainer<mush::imageBuffer> buffer3;
};

#endif
