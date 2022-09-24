//
//  trayraceUpsample.cpp
//  video-mush
//
//  Created by Josh McNamee on 22/09/2015.
//
//

#include "trayraceUpsample.hpp"
#include <Mush Core/opencl.hpp>
#include <Mush Core/integerMapProcess.hpp>

void trayraceUpsample::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 5);
    spatialClear = context->getKernel("spatialClear");
    spatialUpsample = context->getKernel("spatialUpsample");
    
    maps = castToIntegerMap(buffers.begin()[0]);
    geometry = castToImage(buffers.begin()[1]);
    depth = castToImage(buffers.begin()[2]);
    redraw = castToImage(buffers.begin()[3]);
    lowres = castToImage(buffers.begin()[4]);
    
    geometry->getParams(_width, _height, _size);
    
    addItem(context->floatImage(_width, _height));
    
    spatialClear->setArg(0, _getImageMem(0));
    
    
    spatialUpsample->setArg(0, _getImageMem(0));
    spatialUpsample->setArg(1, _getImageMem(0));
    spatialUpsample->setArg(10, scale);
    // !
    
    setParams(0, 0.04, 0.01, 10.0);
    
    queue = context->getQueue();
}

void trayraceUpsample::process() {
    cl::Event event;
    
    inLock();
    
    queue->enqueueNDRangeKernel(*spatialClear, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();

	bool released = false;

	auto re = redraw->outLock();
	if (re == nullptr) {
		released = true;
	}

    auto map = maps->outLock();
    if (map == nullptr) {
		released = true;
    }
    
    auto geom = geometry->outLock();
    if (geom == nullptr) {
		released = true;
    }
    
    auto d = depth->outLock();
    if (d == nullptr) {
		released = true;
    }
    
    auto lr = lowres->outLock();
    if (lr == nullptr) {
		released = true;
    }

	if (released) {
		maps->outUnlock();
		release();
		return;
	}

    spatialUpsample->setArg(2, lr.get_image());
    
    count = (count+1) % 16;
    
    /*
     unsigned char grid[] =  { 6,  5,  8,  12,
     15,  1,  9, 13,
     0, 10,  7,  4,
     14,  3, 11,  2 };
     
     float xdiff2 =  (float)((grid[count] % 4) % scale);
     float ydiff2 =  (float)(((grid[count] - (grid[count] % 4))/4 ) % scale);
     std::stringstream strm;
     strm << "Upsample - xdiff: " << xdiff2 <<" ydiff: " << ydiff2;
     putLog(strm.str());
     */
    spatialUpsample->setArg(8, count);
    
    
    
    spatialUpsample->setArg(3, geom.get_image());
    spatialUpsample->setArg(4, d.get_image());
    spatialUpsample->setArg(5, map.get_buffer());
    spatialUpsample->setArg(9, re.get_buffer());
    
    // BLOCK 'O' FIVE /*FOUR*/
    
    spatialUpsample->setArg(6, (char)-1);
    spatialUpsample->setArg(7, (char)0);
    queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    spatialUpsample->setArg(6, (char)1);
    spatialUpsample->setArg(7, (char)0);
    queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    spatialUpsample->setArg(6, (char)0);
    spatialUpsample->setArg(7, (char)-1);
    queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    spatialUpsample->setArg(6, (char)0);
    spatialUpsample->setArg(7, (char)1);
    queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    
    spatialUpsample->setArg(6, (char)0);
    spatialUpsample->setArg(7, (char)0);
    queue->enqueueNDRangeKernel(*spatialUpsample, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    // END
    
    lowres->outUnlock();
    redraw->outUnlock();
    depth->outUnlock();
    geometry->outUnlock();
    maps->outUnlock();
    
    inUnlock();
}


void trayraceUpsample::setParams(int count, float geomWeight, float depthWeight, float kWeight) {
    
    spatialUpsample->setArg(8, count-1);
    spatialUpsample->setArg(11, geomWeight);
    spatialUpsample->setArg(12, depthWeight);
    spatialUpsample->setArg(13, kWeight);
    this->count = count-1;
    
}
