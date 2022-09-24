//
//  singleEXRInput.hpp
//  video-mush
//
//  Created by Josh McNamee on 19/03/2015.
//
//

#ifndef video_mush_singleEXRInput_hpp
#define video_mush_singleEXRInput_hpp

#include <Mush Core/frameGrabber.hpp>

namespace mush {
    class opencl;
    class singleEXRInput : public mush::frameGrabber {
    public:
        singleEXRInput(std::string p = "") : mush::frameGrabber(mush::inputEngine::singleEXRInput), path(p) {
            setTagInGuiName("Single EXR Loop Input");
        }
        
        ~singleEXRInput() {
            
        }
        
        void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
        
        void getDetails(mush::core::inputConfigStruct &config) override;
        
        void gather() override;
        
    protected:
        
        virtual void inUnlock() override;
        
        virtual void outUnlock() override;
    protected:
    private:
        std::string path = "";
    };

}
#endif
