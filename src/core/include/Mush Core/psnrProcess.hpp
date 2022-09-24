//
//  psnrProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 20/01/2015.
//
//

#ifndef video_mush_psnrProcess_hpp
#define video_mush_psnrProcess_hpp

//#define CPU_PSNR 1

#include "mush-core-dll.hpp"
#include "processNode.hpp"
#include "opencl.hpp"
#include "metricReporter.hpp"
#include "imageProcess.hpp"
#include "imageProcessMetric.hpp"
#include "registerContainer.hpp"

namespace mush{
    class imageBuffer;
    class ringBuffer;
}

namespace mush {
	class psnrProcess : public imageProcessMetric {
    public:
        
		enum class type {
			linear,
			log,
            pu
		};

		MUSHEXPORTS_API psnrProcess(type t, float maxValue, const char * label = "");
        
		MUSHEXPORTS_API ~psnrProcess() {
            
        }
        
		MUSHEXPORTS_API virtual void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
		MUSHEXPORTS_API void process() override;
        
        //MUSHEXPORTS_API void report() override;
        
		//MUSHEXPORTS_API void finalReport() override;

		std::string get_title_format_string() const override {
			return " %10s | ";
		}

		std::string get_format_string() const override {
			return " %4.4fdb | ";
		}
		
		mush::metric_value get_last() const override;
		//float get_final() const override;

		type get_type() const {
			return _t;
		}

    private:
		float get_mse(cl::Kernel * kernel, const float max_squared) const;
        
#ifdef CPU_PSNR
        double cpu_psnr();
#endif
        
        mush::registerContainer<mush::imageBuffer> buffer;
        mush::registerContainer<mush::imageBuffer> buffer2;
        float _max_value = 1.0f;
		cl::Kernel * squared_error = nullptr;
        cl::Kernel * psnr_diff = nullptr;
		cl::Kernel * psnr_sum = nullptr;
        cl::Buffer * output = nullptr;
        float * downfloat = nullptr;
#ifdef CPU_PSNR
        float * test_image_original = nullptr;
        float * test_image_input = nullptr;
        cl::Kernel * scale_image = nullptr;
        cl::Image2D * scaled_image = nullptr;
#endif
        cl::CommandQueue * queue = nullptr;
        
        const char * _label = "";

		std::vector<float> psnrs;
        std::vector<double> cpu_psnrs;

		type _t;
		float _max_squared;
        
        cl::Buffer * pu_map = nullptr;
    };
}

#endif

