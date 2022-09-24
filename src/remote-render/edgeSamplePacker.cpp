//
//  edgeSamplePacker.cpp
//  video-mush
//
//  Created by Josh McNamee on 26/10/2016.
//
//

#include "edgeSamplePacker.hpp"


edgeSamplePacker::edgeSamplePacker() : mush::ringBuffer(), mush::processNode(), mush::metricReporter() {
    
}

edgeSamplePacker::~edgeSamplePacker() {
    
}

void edgeSamplePacker::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    
    assert(buffers.size() == 2);
    
    _edge_samples = castToImage(buffers.begin()[0]);
    _edge_samples->getParams(_width, _height, _size);
    _sample_map = castToIntegerMap(buffers.begin()[1]);
    
    
    addItem(&a);
    addItem(&b);
    
    _host_buffer_samples = (cl_float2 *)context->hostReadBuffer(_width*_height*sizeof(cl_float2));
    _host_buffer_map = context->hostReadBuffer(_width*_height*1*sizeof(uint8_t));
    
    queue = context->getQueue();
}

void edgeSamplePacker::process() {
    auto& sample_vec = *(std::vector<uint16_t> *)inLock().get_pointer();
    sample_vec.clear();
    auto edge_image = _edge_samples->outLock();
    auto edge_map = _sample_map->outLock();
    
    if (edge_image == nullptr) {
		release();
        return;
    }
    if (edge_map == nullptr) {
		release();
        return;
    }
    
    cl::Event event;
    
    queue->enqueueReadBuffer(edge_map.get_buffer(), CL_TRUE, 0, _width*_height*1*sizeof(uint8_t), _host_buffer_map, NULL, &event);
    event.wait();
    
    cl::size_t<3> origin, region;
    origin[0] = 0; origin[1] = 0; origin[2] = 0;
    region[0] = _width; region[1] = _height; region[2] = 1;
    
    queue->enqueueReadImage(edge_image.get_image(), CL_TRUE, origin, region, 0, 0, _host_buffer_samples, NULL, &event);
    event.wait();
    
    for (int j = 0; j < _height; ++j) {
        for (int i = 0; i < _width; ++i) {
            if (_host_buffer_map[_width * j + i] == 1) {
                sample_vec.push_back((uint16_t)_host_buffer_samples[(_width * j + i)].s0);
            }
        }
    }
    
    int length = sample_vec.size() * sizeof(uint16_t);
    
    _lengths.push_back(length);
    
    _edge_samples->outUnlock();
    _sample_map->outUnlock();
    inUnlock();
}

std::string edgeSamplePacker::get_title_format_string() const {
    return " %7s | ";
}

std::string edgeSamplePacker::get_format_string() const {
    return " %7f | ";
}

mush::metric_value edgeSamplePacker::get_last() const {
    mush::metric_value ret;
    ret.type = mush::metric_value_type::i;
    if (!_lengths.empty()) {
        ret.value.integer = _lengths.back();
    } else {
        ret.value.integer = -1;
    }
    return ret;
}

