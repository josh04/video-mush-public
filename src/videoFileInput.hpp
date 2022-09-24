#ifndef VIDEOFILEINPUT_HPP
#define VIDEOFILEINPUT_HPP


#include <stdexcept>
#include <vector>

#include <Mush Core/scrubbableFrameGrabber.hpp>
#include <Mush Core/opencl.hpp>

extern "C" {
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include "ffmpegWrapper.hpp"

#include <Mush Core/SetThreadName.hpp>

#include <mutex>


class VideoFileInput : public mush::scrubbableFrameGrabber, public mush::ffmpegWrapper {
public:
    VideoFileInput(bool isDummyVideo = false);

    ~VideoFileInput();

    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void getDetails(mush::core::inputConfigStruct &config) override;

    void gather() override;
    
    void moveToFrame(int frame) override;
    int getFrameCount() override;
    int getCurrentFrame() override;
    
protected:
private:
    bool doubleHeightFrame = false;
    int _streamCount = 2;
	AVFormatContext * avFormatContext = nullptr;
	AVCodecContext * avLumaCodecContext = nullptr;
	AVCodecContext * avChromaCodecContext = nullptr;
	AVStream * lumaStream = nullptr;
	AVStream * chromaStream = nullptr;

	int chromaId = 0; int lumaId = 0;
    
    int _frameCount = 0;
    std::atomic_int _currentFrame{0};
    std::mutex _scrubMutex;
    bool _scrub = false;
    int _scrubTo = 0;
    
    bool _loop = false;
    
    int64_t first_dts = INT64_MAX;
    int first_stream = -1;
};

#endif
