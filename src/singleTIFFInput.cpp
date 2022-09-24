//
//  singleTIFFInput.cpp
//  video-mush
//
//  Created by Josh McNamee on 22/09/2015.
//
//

#include "singleTIFFInput.hpp"


#include <Mush Core/opencl.hpp>
#include <tiffio.h>

namespace mush {
    
    
    void singleTIFFInput::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        path = std::string(config.inputPath);

        _size = (int)sizeof(uint8_t);
        
        short tiffSampleFormat;
        short tiffSamplesPerPixel;

        
        TIFF* tiff = TIFFOpen(path.c_str(), "r");
        
        TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &_width);
        TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &_height);
        TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &tiffSampleFormat);
        TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &tiffSamplesPerPixel);
        
        if (tiffSampleFormat == 16) {
            _size = (int)sizeof(short);
        }
        
        if (tiffSampleFormat == 32) {
            _size = (int)sizeof(float);
        }
        
        //auto stripSize = TIFFStripSize(tiff);
        //auto numberOfStrips = TIFFNumberOfStrips(tiff);
        //auto top = stripSize * numberOfStrips;
        
        TIFFClose(tiff);
        
        //addItem(context->hostWriteBuffer(numberOfStrips*stripSize));
        addItem(context->hostWriteBuffer(_width*_height*4*_size));
        
        memset(_getMem(0).get_pointer(), 0, _width*_height*4*_size);
    }
    
    void singleTIFFInput::getDetails(mush::core::inputConfigStruct &config) {
        config.inputSize = _size;
        config.inputBuffers = 1;
    }
    
    void singleTIFFInput::gather() {
        unsigned char * ptr = (unsigned char *)inLock().get_pointer();
        TIFFErrorHandler oldhandler;
        oldhandler = TIFFSetWarningHandler(NULL);
        TIFF* tiff = TIFFOpen(path.c_str(), "r");
        TIFFSetWarningHandler(oldhandler);
        
        //TIFFReadRGBAImageOriented(tiff, _width, _height, reinterpret_cast<uint32*>(ptr), 0, ORIENTATION_TOPLEFT);
        auto stripSize = TIFFStripSize(tiff);
        auto numberOfStrips = TIFFNumberOfStrips(tiff);
        for (int strip = 0; strip < numberOfStrips; ++strip) {
            TIFFReadEncodedStrip(tiff, strip, ptr+stripSize*strip, stripSize);
        }
        
        TIFFClose(tiff);
        inUnlock();
    }
    
    void singleTIFFInput::inUnlock() {
        int nx = next;
        mush::imageBuffer::inUnlock();
        empty[nx] = false;
    }
    
    void singleTIFFInput::outUnlock() {
        int nw = now;
        mush::imageBuffer::outUnlock();
        empty[nw] = false;
    }
}
