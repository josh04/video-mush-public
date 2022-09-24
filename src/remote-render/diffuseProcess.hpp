//
//  diffuseProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef video_mush_diffuseProcess_hpp
#define video_mush_diffuseProcess_hpp

#include <Mush Core/registerContainer.hpp>
#include <Mush Core/imageProcess.hpp>

class diffuseProcess : public mush::imageProcess {
public:
    diffuseProcess() : mush::imageProcess() {
        
    }
    
    ~diffuseProcess() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
    void process();
    
private:
    void push_seq(cl::Image2D * top, cl::Image2D * bottom, const unsigned int &width, const unsigned int &height);
    void pull_seq(cl::Image2D * bottom, cl::Image2D * top, const unsigned int &bottom_width, const unsigned int &bottom_height, const unsigned int &top_width, const unsigned int &top_height);
    
    cl::CommandQueue * queue = nullptr;
    
    cl::Kernel * copy = nullptr;
    cl::Kernel * regular_samples = nullptr;
    cl::Kernel * push = nullptr;
    cl::Kernel * pull = nullptr;
    
    cl::Kernel * pull_copy = nullptr;
    cl::Kernel * pull_diag = nullptr;
    cl::Kernel * pull_horiz = nullptr;
    cl::Kernel * final_correct = nullptr;
    
    cl::Image2D * first = nullptr, * first_copy = nullptr,
                * second = nullptr, * second_copy = nullptr,
                * third = nullptr, * third_copy = nullptr,
                * fourth = nullptr, * fourth_copy = nullptr,
                * fifth = nullptr, * fifth_copy = nullptr, * scratch = nullptr;
    mush::registerContainer<mush::imageBuffer> buffer;
    mush::registerContainer<mush::imageBuffer> _regular_samples;
    unsigned int _reg_width, _reg_height;
    
    unsigned int first_width, first_height;
    unsigned int second_width, second_height;
    unsigned int third_width, third_height;
    unsigned int fourth_width, fourth_height;
    unsigned int fifth_width, fifth_height;
    
    
};

#endif
