#ifndef MOTION_REPROJECTION_HPP
#define MOTION_REPROJECTION_HPP

#include <Mush Core/imageProcess.hpp>
#include <Mush Core/registerContainer.hpp>

namespace cl {
    class Kernel;
    class CommandQueue;
}

class motionReprojection : public mush::imageProcess {
public:
    motionReprojection();
    ~motionReprojection();
    
    void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
    
    void process() override;
    
    void set_update_previous(bool update) {
        update_previous = update;
    }
    
    std::shared_ptr<azure::Eventable> get_eventable() const override {
        if (_gl.get() != nullptr) {
            return _gl->get_eventable();
        } else {
            return nullptr;
        }
    }
    
private:
    cl::Kernel * upscale = nullptr;
    cl::Kernel * clear = nullptr;
    cl::Kernel * motion_reprojection = nullptr;
	cl::Kernel * motion_preprocess = nullptr;
    cl::Kernel * copy = nullptr;
    
    cl::Image2D * _previous = nullptr;
    
    mush::registerContainer<mush::imageBuffer> _input;
    mush::registerContainer<mush::imageBuffer> _motion;
    mush::registerContainer<mush::imageBuffer> _small;
    mush::registerContainer<mush::imageBuffer> _depth;
    
    std::shared_ptr<mush::imageBuffer> _gl = nullptr;
    
    bool update_previous = false;
};

#endif
