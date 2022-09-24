//
//  jbig2EncodeProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 25/10/2016.
//
//

#ifndef jbig2EncodeProcess_hpp
#define jbig2EncodeProcess_hpp

#include <memory>
#include <vector>
#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/imageProcess.hpp>
#include <Mush Core/metricReporter.hpp>
#include <Mush Core/registerContainer.hpp>

class jbig2EncodeProcess : public mush::imageProcess, public mush::metricReporter {
public:
    
    struct jbig2_data {
        void * data;
        int length;
    };
    
    jbig2EncodeProcess();
    ~jbig2EncodeProcess();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
    
    std::string get_title_format_string() const override;
    std::string get_format_string() const override;
    mush::metric_value get_last() const override;
private:
    mush::registerContainer<mush::integerMapBuffer> _input;
    
    uint8_t * _host_buffer = nullptr;
    
    jbig2_data a, b;
    
    std::vector<int> _lengths;
};

#endif /* jbig2EncodeProcess_hpp */
