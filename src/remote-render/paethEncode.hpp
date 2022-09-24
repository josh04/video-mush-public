//
//  paethEncode.hpp
//  video-mush
//
//  Created by Josh McNamee on 26/10/2016.
//
//

#ifndef paethEncode_hpp
#define paethEncode_hpp

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

class paethEncode : public mush::ringBuffer, public mush::processNode, public mush::imageProperties, public mush::metricReporter {
public:
    
    paethEncode();
    ~paethEncode();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
    
    void process() override;
    
    
    std::string get_title_format_string() const override;
    std::string get_format_string() const override;
    mush::metric_value get_last() const override;
private:
    mush::registerContainer<mush::integerMapBuffer> _sample_map;
    mush::registerContainer<mush::imageBuffer> _regular_samples;
    
    cl_float4 * _host_regular_samples = nullptr;
    
    std::vector<cl_float4> a, b;
    
    std::vector<int> _lengths;
    
    cl::CommandQueue * queue = nullptr;
};

#endif /* paethEncode_hpp */
