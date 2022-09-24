//
//  lzmaEncoder.hpp
//  video-mush
//
//  Created by Josh McNamee on 26/10/2016.
//
//

#ifndef lzmaEncoder_hpp
#define lzmaEncoder_hpp

#include <memory>
#include <vector>
#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/opencl.hpp>
#include <Mush Core/metricReporter.hpp>
#include <Mush Core/registerContainer.hpp>

namespace cl {
    class CommandQueue;
}

class lzmaEncode : public mush::ringBuffer, public mush::processNode, public mush::imageProperties, public mush::metricReporter {
public:
    
    lzmaEncode();
    ~lzmaEncode();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
    
    void process() override;
    
    
    std::string get_title_format_string() const override;
    std::string get_format_string() const override;
    mush::metric_value get_last() const override;
private:
    mush::registerContainer<mush::ringBuffer> _regular_samples;
    
    cl_float * _host_regular_samples = nullptr;
    
    std::vector<uint8_t> a, b;
    
    std::vector<int> _lengths;
    
    cl::CommandQueue * queue = nullptr;
    
    bool _is_edges = false;
};

#endif /* lzmaEncoder_hpp */
