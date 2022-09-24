//
//  waveformCombiner.hpp
//  video-mush
//
//  Created by Josh McNamee on 01/07/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_waveformCombiner_hpp
#define media_encoder_waveformCombiner_hpp

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace cl {
	class CommandQueue;
	class Kernel;
}

class waveformCombiner : public mush::imageProcess {
public:
	waveformCombiner();
    
    ~waveformCombiner() {
        
    }
    
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
	void process() override;
    
private:
    cl::CommandQueue * queue = nullptr;
    cl::Kernel * waveform_combiner = nullptr;
    
    unsigned long number_of_waveforms;
    
    std::vector<mush::registerContainer<mush::ringBuffer>> buffers;
};

#endif


