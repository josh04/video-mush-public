//
//  edgeEncodingExtractBinaryMap.cpp
//  video-mush
//
//  Created by Josh McNamee on 25/10/2016.
//
//

#include "edgeEncodingExtractBinaryMap.hpp"

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>

edgeEncodingExtractBinaryMap::edgeEncodingExtractBinaryMap() : mush::integerMapProcess(1) {
    
}

edgeEncodingExtractBinaryMap::~edgeEncodingExtractBinaryMap() {
    
}

void edgeEncodingExtractBinaryMap::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    
    assert(buffers.size() == 1);
    
    
    _input = castToImage(buffers.begin()[0]);
    _input->getParams(_width, _height, _size);
    
    create_temp_image(context);
    
    addItem((unsigned char *)context->buffer(_width*_height*sizeof(uint8_t)));
    _edge = context->getKernel("edge_binary_map");
    
    queue = context->getQueue();
}

void edgeEncodingExtractBinaryMap::process() {
    
    auto im_ptr = _input->outLock();
    
    if (im_ptr == nullptr) {
		release();
        return;
    }
    
    cl::Event event;
    
    _edge->setArg(0, im_ptr.get_image());
    _edge->setArg(1, _getMem(0).get_buffer());
    _edge->setArg(2, _width);
    
    queue->enqueueNDRangeKernel(*_edge, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    
    event.wait();
    
    _input->outUnlock();
    inUnlock();
}
