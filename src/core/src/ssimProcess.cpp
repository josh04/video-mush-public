//
//  ssimProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 27/12/2015.
//
//

#include "ssimProcess.hpp"

#include <assert.h>
#include <cmath>
#include <sstream>
#include <array>

#include "imageProcess.hpp"
#include "puEncode.hpp"

namespace mush {
    
    float gaussian_parameter(float scale, float x, float y, float x0, float y0, float dev_x, float dev_y) {
        const float x_term = (pow(x - x0, 2.0f))/(2*pow(dev_x, 2.0f));
        const float y_term = (pow(y - y0, 2.0f))/(2*pow(dev_y, 2.0f));
        return scale * exp(-(x_term + y_term));
    }
    
    ssimProcess::ssimProcess(type t, const char * label) : imageProcessMetric(), _label(label), _t(t) {
        /*
        double sum = 0.0;
        
        for (int j = -5; j <= 5; ++j) {
            for (int i = -5; i <= 5; ++i) {
                sum += gaussian_parameter(1.0, i, j, 0, 0, 1.5, 1.5);
            };
        }
        
        
        for (int j = -5; j <= 5; ++j) {
            std::array<float, 11> row;
            for (int i = -5; i <= 5; ++i) {
                row[i+5] = gaussian_parameter(1.0, i, j, 0, 0, 1.5, 1.5) / sum;
            };
            printf("%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f \n",
                   row[0],
                   row[1],
                   row[2],
                   row[3],
                   row[4],
                   row[5],
                   row[6],
                   row[7],
                   row[8],
                   row[9],
                   row[10],
                   row[11]
                   );
        }
         */
        
    }
    
    void ssimProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 2);
        
        scale_image = context->getKernel("scale_image");
        switch (_t) {
            case type::linear:
                _grey = context->getKernel("bt709luminance");
                break;
            case type::pu:
                _grey = context->getKernel("pu_encode");
                pu_map = puEncode::get_table_buffer(context);
                break;
        }
        
        _ssim = context->getKernel("ssim_process");
        _sum = context->getKernel("psnr_sum");
        
        original = castToImage(buffers.begin()[0]);
        original->getParams(_width, _height, _size);
    
        upscale = context->floatImage(_width, _height);
        
        in_grey = context->greyImage(_width, _height);
        orig_grey = context->greyImage(_width, _height);
        
        
        _width -= 5*2;
        _height -= 5*2;
        down_buffer = context->buffer((_width) * (_height) * sizeof(cl_float));
        
        
        if (_t == type::pu) {
            puEncode p;
			max_value = 1e4f;
            range = p.encode(1e4f);
        }
        
        
        input = castToImage(buffers.begin()[1]);
        
        ssim_image = (cl_float *)context->hostReadBuffer(4 * sizeof(cl_float));
        
        queue = context->getQueue();
        
        addItem(context->floatImage(_width, _height));
        
        null();
    }
    
    void ssimProcess::process() {
        inLock();
        auto in = input->outLock();
        if (in == nullptr) {
            //release();
            return;
        }
        auto orig = original->outLock();
        if (orig == nullptr) {
            //release();
            return;
        }
        cl::Event event;
        /*
        scale_image->setArg(0, *in);
        scale_image->setArg(1, *upscale);

        
        queue->enqueueNDRangeKernel(*scale_image, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        */
        
        if (_t == type::pu) {
            _grey->setArg(2, *pu_map);
        }
        _grey->setArg(0, in.get_image());
        _grey->setArg(1, *in_grey);
        queue->enqueueNDRangeKernel(*_grey, cl::NullRange, cl::NDRange(_width+5*2, _height+5*2), cl::NullRange, NULL, &event);
        event.wait();
        
        _grey->setArg(0, orig.get_image());
        _grey->setArg(1, *orig_grey);
        queue->enqueueNDRangeKernel(*_grey, cl::NullRange, cl::NDRange(_width+5*2, _height+5*2), cl::NullRange, NULL, &event);
        event.wait();
        
        _ssim->setArg(0, *orig_grey);
        _ssim->setArg(1, *in_grey);
        _ssim->setArg(2, _getImageMem(0));
        _ssim->setArg(3, *down_buffer);
        _ssim->setArg(4, 5);

		_ssim->setArg(5, max_value);
        _ssim->setArg(6, range);
            
        
        queue->enqueueNDRangeKernel(*_ssim, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
        event.wait();
        
        const int pixel_count = (_width) * (_height);
        int length = (_width) * (_height);
        
        _sum->setArg(0, *down_buffer);
        while (length > 4) {
            _sum->setArg(1, length);
            length = ceil(length / 2.0f);
            _sum->setArg(2, length);
            queue->enqueueNDRangeKernel(*_sum, cl::NullRange, cl::NDRange(length, 1), cl::NullRange, NULL, &event);
            event.wait();
        }
        
        queue->enqueueReadBuffer(*down_buffer, CL_TRUE, 0, sizeof(float)*length, ssim_image, NULL, &event);
        event.wait();
        
        double mssim = 0.0f;
        
        for (int i = 0; i < length; ++i) {
            mssim += ssim_image[i];
        }
        const float avg_ssim = mssim / pixel_count;
        
        
        
        //
        
        /*
         cl::size_t<3> origin;
         cl::size_t<3> region;
         origin[0] = 0; origin[1] = 0; origin[2] = 0;
         region[0] = _width; region[1] = _height; region[2] = 1;
         
         queue->enqueueReadImage(_getImageMem(0), CL_TRUE, origin, region, 0, 0, ssim_image, NULL, &event);
         event.wait();
        
        double mssim = 0.0;
		int cnt = 0;
        for (int i = 0; i < _width * _height; ++i) {
			if (!std::isnan(ssim_image[i*4])) {
				cnt++;
				mssim += ssim_image[i*4];
			}
        }
        
        const float avg_ssim = mssim / (cnt);
        */
        
        std::stringstream strm;
		std::string t = " SSIM: ";
		if (_t == type::pu) {
			t = " puSSIM: ";
		}
        strm << _label << t << avg_ssim;
        setTagInGuiName(strm.str());
        
        ssims.push_back(avg_ssim);
        
        original->outUnlock();
        input->outUnlock();
        inUnlock();
    }
    /*
    void ssimProcess::report() {
        if (!ssims.empty()) {
            double tot = ssims.back();
            
            std::stringstream strm;
            strm << _label << " SSIM: " << tot;
            putLog(strm.str());
        }
        
    }
    
    void ssimProcess::finalReport() {
        double tot = 0.0f;
        for (auto f : ssims) {
            tot += f;
        }
        
        tot = tot / ssims.size();
        
        std::stringstream strm;
        strm << _label << " average SSIM: " << tot << " over " << ssims.size() << " frames.";
        putLog(strm.str());
        
    }
    */
	mush::metric_value ssimProcess::get_last() const {
		mush::metric_value ret;
		ret.type = metric_value_type::f;
		if (!ssims.empty()) {
			ret.value.floating_point = ssims.back();
		} else {
			ret.value.floating_point = 0.0f;
		}
		return ret;
	}
}

