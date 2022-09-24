//
//  debayerProcessor.cpp
//  video-mush
//
//  Created by Josh McNamee on 05/08/2015.
//
//

#include "debayerProcessor.hpp"

#include <Mush Core/frameStepper.hpp>

#include "debayerProcess.hpp"
#include "quarterImageProcess.hpp"
#include "whitePointProcess.hpp"
#include "dualisoProcessor.hpp"
#include "chromaSmooth.hpp"
#include "bt709luminanceProcess.hpp"
#include <Mush Core/fixedExposureProcess.hpp>

#include <array>
#include <sstream>
#include <iomanip>
#include <math.h>

using namespace mush;
extern void SetThreadName(const char * threadName);

debayerProcessor::debayerProcessor(float * whitePoint, rawCameraType camera_type, bool dualISO, float dual_iso_comp_factor, float raw_clamp) : imageProcessor(), _whitePoint(whitePoint), _dualISO(dualISO), _dual_iso_comp_factor(dual_iso_comp_factor), _camera_type(camera_type), _raw_clamp(raw_clamp) {
    
}

debayerProcessor::~debayerProcessor() {
    if (_dual_iso != nullptr) {
        videoMushRemoveEventHandler(_dual_iso);
    }
}

/*

float _bicubic_parameter(float x, float a) {
    float ret;
    float abs_x = (float)fabs(x);
    if (abs_x < 3) {
        ret = 0.125f;//(a + 2.0f) * pow(abs_x, 3.0f) - (a + 3.0f) * pow(abs_x, 2.0f) + 1.0f;
    } else if (abs_x < 5) {
        ret = 0.625f;//a * pow(abs_x, 3.0f) - 5.0f * a * pow(abs_x, 2.0f) + 8.0f * a * abs_x - 4.0f * a;
    } else {
        ret = 0.0f;
    }
    return ret;
}

float bicubic_parameter(float x, float y, float a) {
    const float weight_x = _bicubic_parameter(x, a);
    const float weight_y = _bicubic_parameter(y, a);
    return weight_x * weight_y;
}
*/
void debayerProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    /*
    std::array<std::array<float, 11>, 11> arr;
    
    float _gauss_sum = 0.0f;
    
    const int half_size = 5;
    
    for (int j = -half_size; j <= half_size; ++j) {
        for (int i = -half_size; i <= half_size; ++i) {
            arr[j+5][i+5] = 0.0f;
            int j_ = abs(j) % 2;
            int i_ = abs(i) % 2;
            if (j_ == 1) {
                if (i_ == 0) {
                    
                    const float weight = bicubic_parameter(i, j, -0.5f);
                    _gauss_sum = _gauss_sum + weight;
                    arr[j+5][i+5] = weight;
                }
            } else {
                if (i_ == 1) {
                    
                    const float weight = bicubic_parameter(i, j, -0.5f);
                    _gauss_sum = _gauss_sum + weight;

                    arr[j+5][i+5] = weight;
                }
            }
        }
    }
    
    std::stringstream strm;
    
    for (int j = 0; j < 11; ++j) {
    for (int i = 0; i < 11; ++i) {
        strm << std::setprecision(3) << arr[j][i] << " ";
    }
        strm << std::endl;
    }
    
    std::cout << strm.str();
    
    std::cout << "Sum Weight: " << _gauss_sum << std::endl;
    */
    assert(buffers.size() == 1);
    _input_buffer = castToImage(buffers.begin()[0]);
    
    if (_dualISO) {
        _dual_iso = make_shared<dualisoProcessor>(_dual_iso_comp_factor);
        _dual_iso->init(context, {_input_buffer});
        videoMushAddEventHandler(_dual_iso);
        
        auto guis = _dual_iso->getGuiBuffers();
        _guiBuffers.insert(_guiBuffers.end(), guis.begin(), guis.end());
    }
    
    _debayer = make_shared<debayerProcess>();
    _debayer->setTagInGuiName("Simple Debayer");
    videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(_debayer));
    
    if (_dualISO) {
        auto outputs = _dual_iso->getBuffers();
        _debayer->init(context, outputs[0]);
    } else {
        _debayer->init(context, _input_buffer);
    }
    
    
    
    _quarter_image = make_shared<quarterImageProcess>();
    _quarter_image->setTagInGuiName("Bayer Quarters");
    
    
    
    if (_dualISO) {
        auto outputs = _dual_iso->getBuffers();
        _quarter_image->init(context, outputs[0]);
    } else {
        _quarter_image->init(context, _input_buffer);
    }
    

	cl_float4 white_point = { _whitePoint[0], _whitePoint[1], _whitePoint[2], _whitePoint[3] };
    _white_point = std::make_shared<whitePointProcess>(white_point, _camera_type, _dualISO, _raw_clamp);
    _white_point->setTagInGuiName("White Point");
    _white_point->init(context, _debayer);
    videoMushAddEventHandler(std::dynamic_pointer_cast<azure::Eventable>(_white_point));
    
    
    _chroma_smooth = make_shared<chroma_smooth>();
    _chroma_smooth->init(context, _white_point);
    _chroma_smooth->setTagInGuiName("Chroma Smoothed");
    
    _guiBuffers.push_back(_chroma_smooth);
    
    _stepper = std::make_shared<mush::frameStepper>();
    
    
    _output_dummy = make_shared<mush::fixedExposureProcess>(0.0f);
    _output_dummy->init(context, _chroma_smooth);
    
    _guiBuffers.push_back(_white_point);
    _guiBuffers.push_back(_input_buffer);
    _guiBuffers.push_back(_debayer);
    _guiBuffers.push_back(_quarter_image);
}

const std::vector<std::shared_ptr<mush::ringBuffer>> debayerProcessor::getBuffers() const {
    return {_output_dummy};
}

std::vector<std::shared_ptr<mush::guiAccessible>> debayerProcessor::getGuiBuffers() {
    const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
    _guiBuffers.clear();
    return buffs;
}

void debayerProcessor::process() {
    if (_dual_iso != nullptr) {
        _dual_iso->process();
    }
    _debayer->process();
    _quarter_image->process();
    _white_point->process();
    _chroma_smooth->process();
    _output_dummy->process();
    
    _stepper->process();
}

void debayerProcessor::release() {
    _white_point->release();
    _debayer->release();
    if (_dual_iso != nullptr) {
        _dual_iso->release();
    }
    _chroma_smooth->release();
    _quarter_image->release();
    _output_dummy->release();
}

void debayerProcessor::go() {
    SetThreadName("debayer");
    while (_input_buffer->good()) {
        process();
    }
    release();
}

std::vector<std::shared_ptr<mush::frameStepper>> debayerProcessor::getFrameSteppers() const {
    return {_stepper};
}