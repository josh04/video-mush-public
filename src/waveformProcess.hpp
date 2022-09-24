//
//  waveformProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 29/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_waveformProcess_hpp
#define media_encoder_waveformProcess_hpp

#include <Mush Core/opencl.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

enum class _waveform_channel {
    luma,
    r,
    g,
    b
};

class waveformProcess : public mush::imageProcess {
public:
	waveformProcess(_waveform_channel ch);
    
    ~waveformProcess() {
        
    }
    
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
    
	void process() override;
    
	void setExposure(const float e);
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * waveform_count = nullptr;
    cl::Kernel * waveform_draw = nullptr;
    cl::Kernel * waveform_clear = nullptr;
    
    cl::Buffer * waveform_counts = nullptr;
    
    cl_int * get_waveform_counts = nullptr;
    
    int luma_bins = 0;
    int width_bins = 0;
    
    unsigned int inWidth, inHeight, inSize;
    
    mush::registerContainer<mush::imageBuffer> buffer;
    cl_float4 weight;
    
    float exp = 0.0f;
    
    _waveform_channel _ch;
};

#endif
