//
//  ssimProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 27/12/2015.
//
//

#ifndef ssimProcess_hpp
#define ssimProcess_hpp


#include "mush-core-dll.hpp"
#include "processNode.hpp"
#include "opencl.hpp"
#include "metricReporter.hpp"
#include "imageProcessMetric.hpp"
#include "registerContainer.hpp"

namespace mush{
    class imageBuffer;
    class ringBuffer;
}

namespace mush {
    class ssimProcess : public imageProcessMetric {
    public:
        
        enum class type {
            linear,
            pu
        };
        
        MUSHEXPORTS_API ssimProcess(type t, const char * label = "");
        
        MUSHEXPORTS_API ~ssimProcess() {
            
        }
        
        MUSHEXPORTS_API virtual void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        MUSHEXPORTS_API void process() override;

		std::string get_title_format_string() const override {
			return " %7s | ";
		}

		std::string get_format_string() const override {
			return " %7f | ";
		}

		mush::metric_value get_last() const override;
		//float get_final() const override;

    private:
        
        type _t;
        mush::registerContainer<mush::imageBuffer> input;
        mush::registerContainer<mush::imageBuffer> original;
        
        cl::Kernel * scale_image = nullptr;
        cl::Kernel * _grey = nullptr;
        cl::Kernel * _ssim = nullptr;
        cl::Kernel * _sum = nullptr;
        
        
        cl::Image2D * upscale = nullptr;
        
        cl::Image2D * in_grey = nullptr;
        cl::Image2D * orig_grey = nullptr;
        cl::Buffer * pu_map = nullptr;
        
        cl::Buffer * down_buffer = nullptr;
        
        cl_float * ssim_image = nullptr;
        
        cl::CommandQueue * queue = nullptr;
        
        const char * _label = "";
        
        std::vector<double> ssims;
        
		float max_value = 1.0f;
        float range = 1.0f;
        
        
        
    };
}

#endif /* ssimProcess_hpp */
