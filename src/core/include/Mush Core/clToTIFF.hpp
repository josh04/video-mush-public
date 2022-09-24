//
//  outputTIFF.hpp
//  video-mush
//
//  Created by Josh McNamee on 23/09/2015.
//
//

#ifndef video_mush_outputTIFF_hpp
#define video_mush_outputTIFF_hpp

#include <tiffio.h>
//#include "yuvDepthClampProcess.hpp"
#include "opencl.hpp"


namespace mush {
    class clToTIFF {
    public:
        clToTIFF() {
            
        }
        
        ~clToTIFF() {
            
        }
        
        void init(shared_ptr<mush::opencl> context) {
            this->context = context;
            queue = context->getQueue();
            kernel = context->getKernel("clampFloatToBGR24");
            kernel->setArg(2, 1.0f);
        }
        
        void write(cl_mem buffer, std::string filename, unsigned int width, unsigned int height) {
            kernel->setArg(0, buffer);
            
            auto chars = context->buffer(sizeof(char) * 3 * width * height, CL_MEM_WRITE_ONLY, false);
            kernel->setArg(1, *chars);
            
            try {
                cl::Event event;
                queue->enqueueNDRangeKernel(*kernel, cl::NullRange, cl::NDRange(width, height), cl::NullRange, NULL, &event);
                event.wait();
            } catch (cl::Error& e) {
                std::stringstream strm;
                strm << "CL Error in " << e.what() << ". Error Code: " << e.err();
                putLog(strm.str());
                return;
            }
            
            unsigned char * pixelBuffer = (unsigned char *)malloc(sizeof(unsigned char) * 3 * width * height);
            queue->enqueueReadBuffer(*chars, CL_TRUE, 0, sizeof(char) * 3 * width * height, pixelBuffer);
            
            delete chars;
            
            TIFF * output_tiff = TIFFOpen(filename.c_str(), "w");
            
            int sampleperpixel = 3;
            
            TIFFSetField(output_tiff, TIFFTAG_IMAGEWIDTH, width);
            TIFFSetField(output_tiff, TIFFTAG_IMAGELENGTH, height);
            TIFFSetField(output_tiff, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);
            TIFFSetField(output_tiff, TIFFTAG_BITSPERSAMPLE, 8);
            TIFFSetField(output_tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
            TIFFSetField(output_tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            
            
            tsize_t linebytes = sampleperpixel * width;
            
            TIFFSetField(output_tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(output_tiff, width*sampleperpixel));
            
            for (unsigned int row = 0; row < height; ++row) {
                if (TIFFWriteScanline(output_tiff, pixelBuffer + row*linebytes, row, 0) < 0)
                    break;
            }
            
            TIFFClose(output_tiff);
            
            free(pixelBuffer);
        }
        
    private:
        std::shared_ptr<mush::opencl> context = nullptr;
        cl::CommandQueue * queue = nullptr;
        cl::Kernel * kernel = nullptr;
    };
        
}

#endif
