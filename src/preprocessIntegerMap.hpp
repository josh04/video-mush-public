//
//  preprocessIntegerMap.hpp
//  video-mush
//
//  Created by Josh McNamee on 23/09/2015.
//
//

#ifndef video_mush_preprocessIntegerMap_hpp
#define video_mush_preprocessIntegerMap_hpp

#include <Mush Core/integerMapBuffer.hpp>

namespace mush {
    class frameGrabber;
    class opencl;

class preprocessIntegerMap : public mush::integerMapBuffer {
public:
    preprocessIntegerMap(int depth = 8) : mush::integerMapBuffer(depth) {
        
    }
    
    ~preprocessIntegerMap() {
        
    }
    
    void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::frameGrabber> input, bool toGrayscale = false);
    
    void process();
    
    void gather();
private:
    cl::CommandQueue * _queue = nullptr;
    
    cl::Kernel * _conversion = nullptr;
    cl::Buffer * _temp = nullptr;
    bool _to_grayscale = false;
    
    std::shared_ptr<mush::frameGrabber> _buffer = nullptr;
};

}
#endif
