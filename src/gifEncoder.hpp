//
//  gifEncoder.hpp
//  video-mush
//
//  Created by Josh McNamee on 25/09/2016.
//
//

#ifndef gifEncoder_hpp
#define gifEncoder_hpp

#include <memory>
#include <gif_lib.h>

#include <Mush Core/encoderEngine.hpp>
#include <Mush Core/registerContainer.hpp>

class gifEncoder : public encoderEngine {
public:
    gifEncoder(unsigned int i);
    ~gifEncoder();
    
    
    void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> outBuffer, mush::core::outputConfigStruct config) override;
    
    std::shared_ptr<AVCodecContext> libavformatContext() override { return nullptr; }
    
    
private:
    void gather() override;
    /*
    virtual mush::buffer _getMem(uint8_t i) override {
        return mush::buffer(&done);
    }*/
    const bool done = true;
    
    GifFileType * start_gif(const char * output_path);
    void add_gif_frame(GifFileType * file, uint8_t * data);
    void close_gif(GifFileType * file);
    
	mush::registerContainer<mush::imageBuffer> _input;
    
    cl::Kernel * _copy_image = nullptr;
    cl::Image2D * _downsample = nullptr;
    uint8_t * _host_buffer = nullptr;
    
    cl::CommandQueue * queue = nullptr;
    
    const char * path = nullptr, * filename = nullptr;
    
    const int _stream_number;
    
    bool _wrote_first_frame = false;
    
    int _fps = 5;
    
    size_t _token = -1;
};

#endif /* gifEncoder_hpp */
