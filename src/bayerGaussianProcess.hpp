//
//  gaussianHorizontal.hpp
//  video-mush
//
//  Created by Josh McNamee on 30/11/2015.
//
//

#ifndef gaussianHorizontal_h
#define gaussianHorizontal_h

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace mush {
    class bayerGaussianProcess : public mush::imageProcess {
    public:
        enum class type {
            horizontal,
            vertical,
            two_dimensional
        };
        
        bayerGaussianProcess(type t, float sigma, int half);
        ~bayerGaussianProcess();
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        void process() override;
        
    private:
        float _sigma = 0.2f;
        int _half = 5;
        
        cl::Kernel * _gauss = nullptr;
        type _t = type::horizontal;
        
        mush::registerContainer<mush::imageBuffer> _buffer;
    };
}

#endif /* gaussianHorizontal_h */
