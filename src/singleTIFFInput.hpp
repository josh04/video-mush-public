//
//  singleTIFFInput.hpp
//  video-mush
//
//  Created by Josh McNamee on 22/09/2015.
//
//

#ifndef video_mush_singleTIFFInput_hpp
#define video_mush_singleTIFFInput_hpp


#include <Mush Core/frameGrabber.hpp>

namespace mush {
    class opencl;
    class singleTIFFInput : public mush::frameGrabber {
    public:
        singleTIFFInput() : mush::frameGrabber(mush::inputEngine::singleEXRInput) {
            setTagInGuiName("Single TIFF Loop Input");
        }
        
        ~singleTIFFInput() {
            
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
