//
//  whitePointProcess.hpp
//  video-mush
//
//  Created by Josh McNamee on 05/08/2015.
//
//

#ifndef video_mush_whitePointProcess_hpp
#define video_mush_whitePointProcess_hpp

#include <azure/Event.hpp>
#include <azure/Eventable.hpp>
#include <Mush Core/singleKernelProcess.hpp>
#include "exports.hpp"

#include <OpenEXR/ImathMatrix.h>

namespace mush {
    class whitePointProcess : public mush::singleKernelProcess, public azure::Eventable {
    public:
        whitePointProcess(cl_float4 point, rawCameraType camera_type, bool dual_iso, float clamp) : mush::singleKernelProcess("white_point"), azure::Eventable(), _point(point), _camera_type(camera_type), _dual_iso(dual_iso), clamp(clamp) {
            
        }
        
        ~whitePointProcess() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override {
            singleKernelProcess::init(context, buffers);
            
            auto event =  std::unique_ptr<azure::Event>(new azure::Event("whitePointReport", 0));
            event->setAttribute<float>("red", _point.s0);
            event->setAttribute<float>("green", _point.s1);
            event->setAttribute<float>("blue", _point.s2);
            azure::Events::Push(std::move(event));
            
            if (_camera_type == rawCameraType::mk2) {
                col1 = {0.8924f, -0.1041f,  0.1760f, 0.0f};
                col2 = {0.4351f,  0.6621f, -0.0972f, 0.0f};
                col3 = {0.0505f, -0.1562f,  0.9308f, 0.0f};
            } else {
                col1 = {0.7868f,  0.0092f,  0.1683f, 0.0f};
                col2 = {0.2291f,  0.8615f, -0.0906f, 0.0f};
                col3 = {0.0027f, -0.4752f,  1.2976f, 0.0f};
            }
            
            setArgs();
            
            rocketGUI("whitepoint.rml");
            
        }
        
	bool event(std::shared_ptr<azure::Event> event);
        
    void setArgs() override;
        
    private:
        rawCameraType _camera_type;
        
        cl_float4 col1, col2, col3;
        
        cl_float4 _point;
        
        bool _dual_iso = false;
        
        float clamp = 1.0f;
    };
    
}

#endif
