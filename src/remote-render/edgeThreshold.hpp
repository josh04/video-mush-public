//
//  laplaceEncoderProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_laplaceEncoderProcess_hpp
#define video_mush_laplaceEncoderProcess_hpp

#include <Mush Core/registerContainer.hpp>
#include <Mush Core/imageProcess.hpp>

class edgeThreshold : public mush::imageProcess {
public:
    edgeThreshold(float threshold);
    ~edgeThreshold();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * threshold = nullptr;
    cl::Kernel * clear = nullptr;
    
    const float _threshold = 0.1f;
    
    mush::registerContainer<mush::imageBuffer> laplaceBuffer;
    mush::registerContainer<mush::imageBuffer> imageBuffe;
};

#endif

