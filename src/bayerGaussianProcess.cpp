//
//  gaussianHorizontal.cpp
//  video-mush
//
//  Created by Josh McNamee on 30/11/2015.
//
//

#include <Mush Core/opencl.hpp>
#include "bayerGaussianProcess.hpp"

namespace mush {
    bayerGaussianProcess::bayerGaussianProcess(type t, float sigma, int half) : imageProcess(), _t(t), _sigma(sigma), _half(half) {
        
    }
    
    bayerGaussianProcess::~bayerGaussianProcess() {}
    
    void bayerGaussianProcess::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        assert(buffers.size() == 1);
        switch (_t) {
            default:
            case type::horizontal:
                _gauss = context->getKernel("bayer_gaussian_horizontal");
                break;
            case type::vertical:
                _gauss = context->getKernel("bayer_gaussian_vertical");
                break;
            case type::two_dimensional:
                _gauss = context->getKernel("bayer_gaussian_two_dimensional");
                break;
        }
        
        _buffer = castToImage(buffers.begin()[0]);
        
        _buffer->getParams(_width, _height, _size);
        
        addItem(context->floatImage(_width, _height));
        
        _gauss->setArg(1, _getImageMem(0));
        _gauss->setArg(2, _sigma);
        _gauss->setArg(3, _half);
        
        
        queue = context->getQueue();
    }
    
    void bayerGaussianProcess::process() {
        inLock();
        auto input = _buffer->outLock();
        if (input == nullptr) {
            release();
            return;
        }
        
        cl::Event event;
        
        _gauss->setArg(0, input.get_image());
        
        queue->enqueueNDRangeKernel(*_gauss, cl::NullRange, cl::NDRange(_width/2, _height/2), cl::NullRange, NULL, &event);
        event.wait();
        
        _buffer->outUnlock();
        inUnlock();
    }
}