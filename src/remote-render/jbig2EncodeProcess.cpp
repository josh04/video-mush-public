//
//  jbig2EncodeProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 25/10/2016.
//
//

#include <leptonica/alltypes.h>
#include <jbig2enc.h>

#include <Mush Core/opencl.hpp>
#include "jbig2EncodeProcess.hpp"

jbig2EncodeProcess::jbig2EncodeProcess() : mush::imageProcess(), mush::metricReporter() {
    
}

jbig2EncodeProcess::~jbig2EncodeProcess() {
    
}

void jbig2EncodeProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    
    assert(buffers.size() == 1);
    
    _input = castToIntegerMap(buffers.begin()[0]);
    _input->getParams(_width, _height, _size);
    
    
    addItem((unsigned char *)&a);
    addItem((unsigned char *)&b);
    
    _host_buffer = context->hostReadBuffer(_width*_height*1*sizeof(uint8_t));
    
    queue = context->getQueue();
}

void jbig2EncodeProcess::process() {
    auto ptr = (jbig2_data *)inLock().get_pointer();
    auto im_ptr = _input->outLock();
    
    if (im_ptr == nullptr) {
		release();
        return;
    }
    
    cl::Event event;
    
    queue->enqueueReadBuffer(im_ptr.get_buffer(), CL_TRUE, 0, _width*_height*1*sizeof(uint8_t), _host_buffer, NULL, &event);
    
    event.wait();
    
    int length = 0;
    
//    /*
//     struct Pix
//     {
//     l_uint32             w;           /* width in pixels                   */
//    l_uint32             h;           /* height in pixels                  */
//    l_uint32             d;           /* depth in bits (bpp)               */
//    l_uint32             spp;         /* number of samples per pixel       */
//    l_uint32             wpl;         /* 32-bit words/line                 */
//    l_uint32             refcount;    /* reference count (1 if no clones)  */
//    l_int32              xres;        /* image res (ppi) in x direction    */
//    /* (use 0 if unknown)                */
//    l_int32              yres;        /* image res (ppi) in y direction    */
//    /* (use 0 if unknown)                */
//    l_int32              informat;    /* input file format, IFF_*          */
//    l_int32              special;     /* special instructions for I/O, etc */
//    char                *text;        /* text string associated with pix   */
//    struct PixColormap  *colormap;    /* colormap (may be null)            */
//    l_uint32            *data;        /* the image data                    */
//};
//*/
    
    unsigned int w = (_width + (32 - (_width % 32)))/32;
    
    std::vector<uint32_t> packed;
    for (int j = 0; j < _height; ++j) {
        for (int i = 0; i < w; ++i) {
            uint32_t u = 0;
            for (int r = 0; r < 32; ++r) {
                if (i*32 + r < _width) {
                    uint32_t taken = _host_buffer[j * _width + i*32 + r];
                    taken = (taken & 1) << (31 - r);
                    u = u | taken;
                }
            }
            packed.push_back(u);
        }
    }

    Pix p;
    p.w = w * 32;
    p.h = _height;
    p.d = 1;
    p.spp = 1;
    p.wpl = w;//_width/4; // ?
    p.refcount = 1;
    p.xres = 0;
    p.yres = 0;
    p.informat = IFF_UNKNOWN;
    p.special = 0;
    p.text = "";
    p.colormap = NULL;
    p.data = (uint32_t *)packed.data();

//    jbig2_encode_generic(struct Pix *const bw, const bool full_headers,
//                         const int xres, const int yres,
//                         const bool duplicate_line_removal,
//                         int *const length);
    
    ptr->data = jbig2_encode_generic(&p, true, 0, 0, false, &length);
    ptr->length = length;
    
    _lengths.push_back(length);
    _input->outUnlock();
    inUnlock();
}

std::string jbig2EncodeProcess::get_title_format_string() const {
    return " %7s | ";
}

std::string jbig2EncodeProcess::get_format_string() const {
    return " %7f | ";
}

mush::metric_value jbig2EncodeProcess::get_last() const {
    mush::metric_value ret;
    ret.type = mush::metric_value_type::i;
    if (!_lengths.empty()) {
        ret.value.integer = _lengths.back();
    } else {
        ret.value.integer = -1;
    }
    return ret;
}

