//
//  puEncode.hpp
//  mush-core
//
//  Created by Josh McNamee on 23/02/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#ifndef puEncode_hpp
#define puEncode_hpp

#include <vector>
#include <memory>

namespace cl {
    class Buffer;
}

namespace mush {
    class opencl;
}

class puEncode {
public:
    puEncode();
    ~puEncode();
    
    float encode(float in);
    
    static cl::Buffer * get_table_buffer(std::shared_ptr<mush::opencl> context);
private:
    static float interpolate_table(float in);
    
    static void build_table();
    
    
    static std::vector<std::pair<float, float> > _lut;
};

#endif /* puEncode_hpp */
