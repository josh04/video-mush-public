//
//  integerMapProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 22/08/2014.
//
//

#ifndef video_mush_integerMapProcess_hpp
#define video_mush_integerMapProcess_hpp

#include <assert.h>

#include "integerMapBuffer.hpp"
#include "processNode.hpp"
#include "initNode.hpp"

namespace mush {
    
    static std::shared_ptr<mush::integerMapBuffer> castToIntegerMap(const std::shared_ptr<mush::ringBuffer> in) {
        auto out = std::dynamic_pointer_cast<mush::integerMapBuffer>(in);
        if (out == nullptr) {
            throw std::runtime_error("Error: A non-integer buffer was passed to an integer process.");
        }
        return out;
    }
    
    class integerMapProcess : public integerMapBuffer, public processNode, public initNode {
    public:
        integerMapProcess(int bitDepth = 8) : integerMapBuffer(bitDepth), processNode() {
            
        }
        
        ~integerMapProcess() {
            
        }
        
    private:
    };
    
}

#endif
