//
//  bilateralProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 11/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_bilateralProcess_hpp
#define media_encoder_bilateralProcess_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

class bilateralProcess : public mush::imageProcess {
public:
    bilateralProcess(float sigma_s, float sigma_r, int64_t halfwindow, mush::bilateralMode bilateralMode) : mush::imageProcess(), sigma_s(sigma_s), sigma_r(sigma_r), halfwindow(halfwindow), bilateralMode(bilateralMode) {
        
    }
    
    bilateralProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);

		switch (bilateralMode) {
            default:
		case mush::bilateralMode::bilinear:
			bilateral = context->getKernel("bilinear");
			break;
		case mush::bilateralMode::bilateral:
			bilateral = context->getKernel("bilateral");
			break;
		case mush::bilateralMode::bilateralOpt:
			bilateral = context->getKernel("bilateralOpt");
			bilateral->setArg(5, cl::__local(32 * 32 * sizeof(float)));
			break;
		case mush::bilateralMode::off:
			bilateral = context->getKernel("copyImage");
			break;
		}


        //        luma->setArg(1, *lumaImage);
        
        buffer = castToImage(buffers.begin()[0]);
        
        buffer->getParams(_width, _height, _size);
        
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
        bilateral->setArg(0, input.get_image());
        bilateral->setArg(1, _getImageMem(0)); // Output
        
        if (bilateralMode != mush::bilateralMode::off) {
            bilateral->setArg(2, 1.0f / (2.0f * sigma_s*sigma_s)); // sigma s, range 0.1 - 16.0 (default, 0.4f). 1.0f / (2.0f * sigmaS*sigmaS)
            bilateral->setArg(3, 1.0f / (2.0f * sigma_r*sigma_r)); // sigma r, range 0.1 - 1.0 - inf (default, 0.2f). 1.0f / (2.0f * sigmaR*sigmaR)
            bilateral->setArg(4, halfwindow); // Half window, range 0 - inf (default, 1-8)
        }
        
        
        
		try {
			queue->enqueueNDRangeKernel(*bilateral, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
			event.wait();
		} catch (cl::Error &e) {
			std::cerr << e.what() << std::endl;
			return;
		} catch (std::exception &e) {
			putLog("oh shit");
		}
        
        buffer->outUnlock();
        inUnlock();
    }
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * bilateral = nullptr;
    float sigma_s = 4.0f, sigma_r = 2.0f;
    int halfwindow = 5;
    mush::registerContainer<mush::imageBuffer> buffer;
	mush::bilateralMode bilateralMode = mush::bilateralMode::bilinear;
};

#endif
