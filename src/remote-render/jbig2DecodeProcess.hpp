//
//  jbig2DecodeProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 25/10/2016.
//
//

#ifndef jbig2DecodeProcess_hpp
#define jbig2DecodeProcess_hpp


#include <memory>
#include <vector>
#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/registerContainer.hpp>
#include <Mush Core/imageProcess.hpp>

#include <jbig2.h>

class jbig2DecodeProcess : public mush::integerMapProcess {
public:
    jbig2DecodeProcess();
    ~jbig2DecodeProcess();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
private:
    mush::registerContainer<mush::imageBuffer> _input;
    
    uint8_t * _host_buffer = nullptr;
    
    std::vector<int> _lengths;
    
    Jbig2Ctx * _ctx = nullptr;
};

#endif /* jbig2DecodeProcess_hpp */
