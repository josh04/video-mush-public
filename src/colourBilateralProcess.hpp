//
//  bilateralProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 11/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_colourBilateralProcess_hpp
#define media_encoder_colourBilateralProcess_hpp


#include <Mush Core/opencl.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class colourBilateralProcess : public mush::imageProcess {
public:
	colourBilateralProcess(const float sigma_s, const float sigma_r, const int64_t halfwindow) : mush::imageProcess(), sigma_s(sigma_s), sigma_r(sigma_r), halfwindow(halfwindow) {

	}

	~colourBilateralProcess() {

	}

	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		assert(buffers.size() == 1);
        bilateral = context->getKernel("multiBilinear");

		slic_rgb_to_cielab = context->getKernel("slic_rgb_to_cielab");
		slic_cielab_to_rgb = context->getKernel("slic_cielab_to_rgb");

		buffer = castToImage(buffers.begin()[0]);

		buffer->getParams(_width, _height, _size);

        cielab = context->floatImage(_width, _height);
        
        addItem(context->floatImage(_width, _height));

		queue = context->getQueue();
	}

	void process() {
		cl::Event event;
		inLock();
		auto input = buffer->outLock();
		if (input == nullptr) {
			release();
			return;
		}


		try {
            
            slic_rgb_to_cielab->setArg(0, input.get_image());
            slic_rgb_to_cielab->setArg(1, *cielab);
            
			queue->enqueueNDRangeKernel(*slic_rgb_to_cielab, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();
            
            
            
            bilateral->setArg(0, *cielab);
            bilateral->setArg(1, _getImageMem(0)); // Output
            bilateral->setArg(2, 1.0f / (2.0f * sigma_s*sigma_s)); // sigma s, range 0.1 - 16.0 (default, 0.4f). 1.0f / (2.0f * sigmaS*sigmaS)
            bilateral->setArg(3, 1.0f / (2.0f * sigma_r*sigma_r)); // sigma r, range 0.1 - 1.0 - inf (default, 0.2f). 1.0f / (2.0f * sigmaR*sigmaR)
            bilateral->setArg(4, halfwindow); // Half window, range 0 - inf (default, 1-8)
            
            
			queue->enqueueNDRangeKernel(*bilateral, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();
            
            slic_cielab_to_rgb->setArg(0, _getImageMem(0));
            slic_cielab_to_rgb->setArg(1, _getImageMem(0));
            
			queue->enqueueNDRangeKernel(*slic_cielab_to_rgb, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();
		} catch (std::exception &e) {
			putLog("Exception in colour bilateral process");
			kill();
		}

		buffer->outUnlock();
		inUnlock();
	}

private:
	cl::CommandQueue * queue = nullptr;
	cl::Kernel * bilateral = nullptr;
	cl::Kernel * slic_rgb_to_cielab = nullptr;
	cl::Kernel * slic_cielab_to_rgb = nullptr;

	cl::Image2D * cielab = nullptr;

	float sigma_s = 0.4f, sigma_r = 0.2f;
	int halfwindow = 5;
	mush::registerContainer<mush::imageBuffer> buffer;
};

#endif
