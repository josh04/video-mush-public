//
//  outputEngine.hpp
//  video-mush
//
//  Created by Josh McNamee on 07/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_outputEngine_hpp
#define media_encoder_outputEngine_hpp

#include "encoderEngine.hpp"

class outputEngine {
public:
    outputEngine() {
        
    }
    
    ~outputEngine() {
        
    }
    
    virtual void init(std::vector<std::shared_ptr<encoderEngine>> encoderEngines, mush::core::outputConfigStruct config) = 0;
    
    virtual void gather() = 0;
    
    void startThread() {
        SetThreadName("outputEngine");
        gather();
    }
    
private:
};

#endif
