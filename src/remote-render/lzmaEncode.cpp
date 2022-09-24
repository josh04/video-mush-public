//
//  lzmaEncoder.cpp
//  video-mush
//
//  Created by Josh McNamee on 26/10/2016.
//
//

#include <lzma.h>

#include "edgeSamplePacker.hpp"
#include "lzmaEncode.hpp"


lzmaEncode::lzmaEncode() : mush::ringBuffer(), mush::processNode(), mush::metricReporter(){
    
}

lzmaEncode::~lzmaEncode() {
    
}

void lzmaEncode::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    
    assert(buffers.size() == 1);
    
    _regular_samples = buffers.begin()[0];
    
    addItem((unsigned char *)&a);
    addItem((unsigned char *)&b);
    
    if (std::dynamic_pointer_cast<edgeSamplePacker>(buffers.begin()[0]) != nullptr) {
        _is_edges = true;
        
    } else {
        
        castToImage(buffers.begin()[0])->getParams(_width, _height, _size);
        
        _host_regular_samples = (cl_float *)context->hostReadBuffer(_width*_height*sizeof(cl_float));
    }
    
    
    queue = context->getQueue();
}

void lzmaEncode::process() {
    auto& out_vec = *(std::vector<uint8_t> *)inLock().get_pointer();
    out_vec.clear();
    
    size_t data_length = 0;
    
    if (!_is_edges) {
        auto reg_image = _regular_samples->outLock();
        
        if (reg_image == nullptr) {
			release();
            return;
        }
        
        cl::Event event;
        
        cl::size_t<3> origin, region;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = _width; region[1] = _height; region[2] = 1;
        
        queue->enqueueReadImage(reg_image.get_image(), CL_TRUE, origin, region, 0, 0, _host_regular_samples, NULL, &event);
        event.wait();
        data_length = _width*_height*sizeof(cl_float);
    } else {
        auto buf = _regular_samples->outLock();
        
        if (buf == nullptr) {
			release();
            return;
        }

		auto edg_vec_ = (std::vector<cl_float> *)buf.get_pointer();
        std::vector<cl_float>& edg_vec = *edg_vec_;
        
        _host_regular_samples = edg_vec.data();
        data_length = edg_vec.size() * sizeof(cl_float);
        
    }
    
    lzma_check check = LZMA_CHECK_CRC32; // PLAY FAST, LOOSE AND DANGEROUS
    lzma_stream strm = LZMA_STREAM_INIT;
    
    auto ret = lzma_easy_encoder (&strm, 9 | LZMA_PRESET_EXTREME, check);
    
    strm.avail_in = data_length;
    strm.next_in = (const uint8_t *)_host_regular_samples;
    
    const int out_size = 4096;
    std::vector<uint8_t> out_buffer(out_size);
    
    while (strm.avail_out == 0) {
        strm.next_out = out_buffer.data();
        strm.avail_out = out_size;
        
        ret = lzma_code(&strm, LZMA_FINISH);
        
        int output_length = out_size - strm.avail_out;
        
        out_vec.insert(out_vec.end(), out_buffer.begin(), out_buffer.begin() + output_length);
        
    }
    
    lzma_end(&strm);
    
    
    int length = out_vec.size();
    
    _lengths.push_back(length);
    
    _regular_samples->outUnlock();
    inUnlock();
}

std::string lzmaEncode::get_title_format_string() const {
    return " %7s | ";
}

std::string lzmaEncode::get_format_string() const {
    return " %7f | ";
}

mush::metric_value lzmaEncode::get_last() const {
    mush::metric_value ret;
    ret.type = mush::metric_value_type::i;
    if (!_lengths.empty()) {
        ret.value.integer = _lengths.back();
    } else {
        ret.value.integer = -1;
    }
    return ret;
}

