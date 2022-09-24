//
//  edgeSamplePacker.hpp
//  video-mush
//
//  Created by Josh McNamee on 26/10/2016.
//
//

#ifndef edgeSamplePacker_hpp
#define edgeSamplePacker_hpp


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

class edgeSamplePacker : public mush::ringBuffer, public mush::processNode, public mush::imageProperties, public mush::metricReporter {
public:
    
    edgeSamplePacker();
    ~edgeSamplePacker();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers);
    
    void process() override;
    
    
    std::string get_title_format_string() const override;
    std::string get_format_string() const override;
    mush::metric_value get_last() const override;
private:
    mush::registerContainer<mush::integerMapBuffer> _sample_map;
    mush::registerContainer<mush::imageBuffer> _edge_samples;
    
    cl_float2 * _host_buffer_samples = nullptr;
    uint8_t * _host_buffer_map = nullptr;
    
    std::vector<uint16_t> a, b;
    
    std::vector<int> _lengths;
    
    cl::CommandQueue * queue = nullptr;
};



#endif /* edgeSamplePacker_hpp */
