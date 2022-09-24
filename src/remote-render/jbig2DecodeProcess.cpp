//
//  jbig2DecodeProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 25/10/2016.
//
//

#include "jbig2DecodeProcess.hpp"
#include "jbig2EncodeProcess.hpp"

#include <jbig2.h>

#include <Mush Core/opencl.hpp>
#include <Mush Core/mushLog.hpp>

jbig2DecodeProcess::jbig2DecodeProcess() : mush::integerMapProcess(1) {
    
}

jbig2DecodeProcess::~jbig2DecodeProcess() {
    
}

typedef int (*Jbig2ErrorCallback) (void *data,
const char *msg, Jbig2Severity severity,
int32_t seg_idx);

int error_callback(void *data, const char *msg, Jbig2Severity severity, int32_t seg_idx) {
    if (severity == JBIG2_SEVERITY_FATAL) {
        throw std::runtime_error(std::string(msg));
    } else if (severity == JBIG2_SEVERITY_WARNING) {
        putLog(msg);
    }
    return 0;
}

void jbig2DecodeProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    
    assert(buffers.size() == 1);
    
    _input = castToImage(buffers.begin()[0]);
    _input->getParams(_width, _height, _size);
    
    addItem((unsigned char *)context->buffer(_width*_height*sizeof(cl_uchar)));
    
    _host_buffer = context->hostWriteBuffer(_width*_height*1*sizeof(uint8_t));
    
    queue = context->getQueue();

    /*
     Jbig2Ctx *jbig2_ctx_new (Jbig2Allocator *allocator,
     Jbig2Options options,
     Jbig2GlobalCtx *global_ctx,
     Jbig2ErrorCallback error_callback,
     void *error_callback_data);
     void jbig2_ctx_free (Jbig2Ctx *ctx);
     */
    
}

void jbig2DecodeProcess::process() {
    inLock();
	auto buf = _input->outLock();
    
    if (buf == nullptr) {
		release();
        return;
    }
	auto im_ptr = (jbig2EncodeProcess::jbig2_data *)buf.get_pointer();
    
    auto in_data = im_ptr->data;
    auto in_length = im_ptr->length;
    
    _ctx = jbig2_ctx_new(NULL, (Jbig2Options)0, NULL, error_callback, NULL);
    
    int err = jbig2_data_in(_ctx, (const unsigned char *)in_data, in_length);
    
    auto img = jbig2_page_out(_ctx);
    if (img != NULL) {
        //memcpy(_host_buffer, img->data, _width*_height*1*sizeof(uint8_t));
        
        //unsigned int w = (_width + (32 - (_width % 32)));
        
        
        for (int j = 0; j < _height; ++j) {
            for (int i = 0; i < _width; ++i) {
                int along = (i % 8);
                int reg = (i - along) / 8;
                uint8_t relevant = img->data[j * img->stride + reg];
                _host_buffer[j * _width + i] = (relevant >> (7 - along)) & 1;
            }
        }
        
        cl::Event event;
        
        queue->enqueueWriteBuffer(_getMem(0).get_buffer(), CL_TRUE, 0, _width*_height*1*sizeof(uint8_t), _host_buffer, NULL, &event);
        
        event.wait();
        
        jbig2_release_page(_ctx, img);
    }
    
    //*ptr = jbig2_encode_generic(&p, true, 0, 0, false, &length);
    
    free(in_data);
    
    if (_ctx != nullptr) {
        jbig2_ctx_free(_ctx);
    }
    
    _input->outUnlock();
    inUnlock();
}


