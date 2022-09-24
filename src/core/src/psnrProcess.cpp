//
//  psnrProcess.cpp
//  video-mush
//
//  Created by Josh McNamee on 29/06/2015.
//
//
#include <assert.h>
#include <cmath>
#include <sstream>

#include "psnrProcess.hpp"
#include "imageProcess.hpp"
#include "puEncode.hpp"

namespace mush {

psnrProcess::psnrProcess(type t, float maxValue, const char * label) : imageProcessMetric(), _max_value(maxValue), _label(label), _t(t) {
    
}

void psnrProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    assert(buffers.size() == 2);
	switch (_t) {
	case type::linear:
		squared_error = context->getKernel("squared_error");
		_max_squared = _max_value*_max_value;
		break;
	case type::log:
            squared_error = context->getKernel("log_squared_error");
            _max_value = log(1e4);
            _max_squared = _max_value*_max_value;
            break;
    case type::pu:
        {
        squared_error = context->getKernel("squared_error_pu");
            puEncode p;
            _max_value = p.encode(1e4);
            _max_squared = _max_value*_max_value;
        }
        break;

	}

    psnr_diff = context->getKernel("psnr_diff");
	psnr_sum = context->getKernel("psnr_sum");
    
#ifdef CPU_PSNR
    scale_image = context->getKernel("scale_image");
#endif
    buffer = castToImage(buffers.begin()[0]);
    
    buffer->getParams(_width, _height, _size);
    
    
#ifdef CPU_PSNR
    scaled_image = context->floatImage(_width, _height);
    scale_image->setArg(1, *scaled_image);
#endif
   
    buffer2 = castToImage(buffers.begin()[1]);
    
    downfloat = (float *)context->hostReadBuffer(4 * sizeof(float));
    
#ifdef CPU_PSNR
    test_image_input = (float *)context->hostReadBuffer(_width * _height * 4 * sizeof(float));
    test_image_original = (float *)context->hostReadBuffer(_width * _height * 4 * sizeof(float));
#endif
	output = context->buffer(_width * _height * sizeof(float));
    
    if (_t == type::pu) {
        pu_map = puEncode::get_table_buffer(context);
    }
    
    queue = context->getQueue();
    
    addItem(context->floatImage(_width, _height));
    
    null();
}

void psnrProcess::process() {
    
    auto input = buffer->outLock();
    if (input == nullptr) {
        //release();
        return;
    }
    auto input2 = buffer2->outLock();
    if (input2 == nullptr) {
        //release();
        return;
    }
    cl::Event event;
    
    cl::size_t<3> origin;
    cl::size_t<3> region;
    origin[0] = 0; origin[1] = 0; origin[2] = 0;
    region[0] = _width; region[1] = _height; region[2] = 1;
    
#ifdef CPU_PSNR
    queue->enqueueReadImage(*input, CL_TRUE, origin, region, 0, 0, test_image_input, NULL, &event);
    event.wait();
    scale_image->setArg(0, *input2);
    
    
    queue->enqueueNDRangeKernel(*scale_image, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();
    
    queue->enqueueReadImage(*scaled_image, CL_TRUE, origin, region, 0, 0, test_image_original, NULL, &event);
    event.wait();
    
    double cpu = cpu_psnr();
    cpu_psnrs.push_back(cpu);
#endif
    
    squared_error->setArg(0, input.get_image());
    squared_error->setArg(1, input2.get_image());
    squared_error->setArg(2, *output);
    squared_error->setArg(3, _max_value);
	/*
	log_squared_error->setArg(0, input.get_image());
	log_squared_error->setArg(1, *input2);
	log_squared_error->setArg(2, *output);
    */
    psnr_diff->setArg(0, input.get_image());
    psnr_diff->setArg(1, input2.get_image());
    psnr_diff->setArg(2, _getImageMem(0));
    
    
    if (_t == type::pu) {
        squared_error->setArg(4, *pu_map);
    }
    
    inLock();
    queue->enqueueNDRangeKernel(*psnr_diff, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
    event.wait();
    inUnlock();

    queue->enqueueNDRangeKernel(*squared_error, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
    event.wait();

	const float n_psnr_output = get_mse(squared_error, _max_squared);
	
	psnrs.push_back(n_psnr_output);

    buffer2->outUnlock();
    buffer->outUnlock();
    
}

float psnrProcess::get_mse(cl::Kernel * kernel, const float max_squared) const {
	cl::Event event;
	const float pixel_count = _width * _height;

	queue->enqueueNDRangeKernel(*kernel, cl::NullRange, cl::NDRange(_width, _height), cl::NullRange, NULL, &event);
	event.wait();

	int length = _width * _height;

	psnr_sum->setArg(0, *output);
	while (length > 4) {
		psnr_sum->setArg(1, length);
		length = ceil(length / 2.0f);
		psnr_sum->setArg(2, length);
		queue->enqueueNDRangeKernel(*psnr_sum, cl::NullRange, cl::NDRange(length, 1), cl::NullRange, NULL, &event);
		event.wait();
	}

	queue->enqueueReadBuffer(*output, CL_TRUE, 0, sizeof(float)*length, downfloat, NULL, &event);
	event.wait();

	double sum_mse = 0.0f;

	for (int i = 0; i < length; ++i) {
		sum_mse += downfloat[i];
	}
	const float avg_mse = sum_mse / pixel_count;

	const float psnr_output = 10.0 * log10(max_squared / avg_mse);

	return psnr_output;
}

/*
void psnrProcess::report() {
    if (!psnrs.empty()) {
        double tot = psnrs.back();

        std::stringstream strm;
        strm << _label << " PSNR: " << tot;
        putLog(strm.str());
    }
    
#ifdef CPU_PSNR
    if (!cpu_psnrs.empty()) {
        double tot = cpu_psnrs.back();
        
        std::stringstream strm;
        strm << _label << " CPU PSNR: " << tot;
        putLog(strm.str());
    }
#endif
}
    
void psnrProcess::finalReport() {
	double tot = 0.0f;
	int cnt = 0;
	for (auto f : psnrs) {
		if (!std::isnan(f) && !std::isinf(f)) {
			tot += f;
			cnt++;
		}
	}
	//double ltot = 0.0f;
	//for (auto f : logpsnrs) {
	//	ltot += f;
	//}
	tot = tot / cnt;
	//ltot = ltot / logpsnrs.size();
    
#ifdef CPU_PSNR
    double cpu_total = 0.0;
    for (auto& d : cpu_psnrs) {
        cpu_total += d;
    }
    cpu_total = cpu_total / cpu_psnrs.size();
#endif
	std::stringstream strm;
	strm << _label << " average PSNR: " << tot << " over " << psnrs.size() << " frames.";
	putLog(strm.str());
	//std::stringstream strm2;
	//strm2 << _label << " average logPSNR: " << ltot << " over " << logpsnrs.size() << " frames.";
    //putLog(strm2.str());
    
    
#ifdef CPU_PSNR
    std::stringstream strm3;
    strm3 << _label << " average cpu psnr: " << cpu_total << " over " << cpu_psnrs.size() << " frames.";
    putLog(strm3.str());
#endif
}
*/
#ifdef CPU_PSNR
    double psnrProcess::cpu_psnr() {
        double sum_error = 0.0;
        
        const double max_val = 1.0;
        
        for (int j = 0; j < _height; ++j) {
            for (int i = 0; i < _width; ++i) {
                sum_error += pow(std::min((double)test_image_original[j*_width + i], max_val)
                               - std::min((double)test_image_input[j*_width + i], max_val), 2.0);
            }
        }
        
        double mse = sum_error / (_width * (double)_height);
        
        double psnr = 10.0 * log10(pow(max_val, 2.0) / mse);
        return psnr;
    }
#endif

	mush::metric_value psnrProcess::get_last() const {
		mush::metric_value ret;
		ret.type = metric_value_type::f;
		if (!psnrs.empty()) {
			ret.value.floating_point = psnrs.back();
		} else {
			ret.value.floating_point = 0.0f;
		}
		return ret;
	}
}
