#ifndef HDRJPEGINPUT_HPP
#define HDRJPEGINPUT_HPP

#include <jpeglib.h>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <Mush Core/mushLog.hpp>
#include <Mush Core/opencl.hpp>
#include <Mush Core/frameGrabber.hpp>


using namespace boost::filesystem;

class hdrJpegInput : public mush::frameGrabber {
public:
	hdrJpegInput() : mush::frameGrabber(mush::inputEngine::jpegInput) {

	}

	~hdrJpegInput() {

	}

	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
		input = config.inputPath;
		_width = config.inputWidth;
		_height = config.inputHeight;
		_size = 1;
		queue = context->getQueue();

		for (int i = 0; i < config.inputBuffers*config.exposures; i++) {
			addItem(reinterpret_cast<unsigned char *>(context->hostWriteBuffer(_width*_height*4*_size)));
		}
        
	}
    
    void getDetails(mush::core::inputConfigStruct &config) override {
        config.inputSize = 2;
        config.inputBuffers = 1;
    }

	void gather() {
        
        SetThreadName("folderInput");
        
		int frameNumber = 0;
		directory_iterator end_itr;

		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;

		FILE * infile;      /* source file */
		JSAMPARRAY buffer;      /* Output row buffer */
		int row_stride;     /* physical row width in output buffer */

		std::string ext, altext;

		ext = ".jpg"; altext = ".jpeg";

		std::string pat;
		std::vector<path> paths;

		paths.empty();
		for (directory_iterator itr(input); itr != end_itr; ++itr) {
			paths.push_back(itr->path());
		}
		sort(paths.begin(), paths.end());
		_numberOfFrames = paths.size();

		for (std::vector<path>::iterator it = paths.begin(); it != paths.end(); ++it) {
			if (it->extension().string() == ext || it->extension().string() == altext) {
				pat = it->string();
				if ((infile = fopen(pat.c_str(), "rb")) == NULL) {
					putLog("Can't open JPEG File: " + pat);
					return;
				}

				cinfo.err = jpeg_std_error(&jerr);
				jpeg_create_decompress(&cinfo);
				jpeg_stdio_src(&cinfo, infile);
				(void) jpeg_read_header(&cinfo, TRUE);

				cinfo.output_gamma = 0.4545;
				(void) jpeg_start_decompress(&cinfo);
				auto buf = inLock();
				if (buf == nullptr) {
					return;
				}
				unsigned char * ptr = (unsigned char *)buf.get_pointer();

				long counter = 0;

				unsigned int scanline_length = _width * 3;
				unsigned int scanline_count = 1;
				while (cinfo.output_scanline < cinfo.output_height) {

					JSAMPROW send = ptr + (cinfo.output_height - scanline_count) * scanline_length;
					(void) jpeg_read_scanlines(&cinfo, &send, 1);
					++scanline_count;
				}

				inUnlock();
				(void) jpeg_finish_decompress(&cinfo);
				jpeg_destroy_decompress(&cinfo);

				fclose(infile);

			}
		}
        release();
		/* And we're done! */
	}
private:
	const char * input;
};

#endif