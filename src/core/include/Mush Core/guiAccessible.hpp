//
//  guiBuffer.hpp
//  video-mush
//
//  Created by Josh McNamee on 02/02/2015.
//
//

#ifndef video_mush_guiBuffer_hpp
#define video_mush_guiBuffer_hpp

#include <string>

#include "mush-core-dll.hpp"
#include "ringBuffer.hpp"
#include "imageProperties.hpp"

#include <memory>
#include <azure/eventable.hpp>

//#include "tagInGui.hpp"

class tagInGui;
namespace cl {
    class Image2D;
    class CommandQueue;
}

namespace mush {
    class MUSHEXPORTS_API guiAccessible : public ringBuffer, public imageProperties {
    public:
        guiAccessible() : ringBuffer(), imageProperties() {
            
        }
        
        ~guiAccessible() {
            
        }
        
        virtual void guiTag(int i, tagInGui * tag);
        
        void passGuiTag(shared_ptr<mush::guiAccessible> pass);
        
        void setTagInGuiRecording(bool set);
        
        void setTagInGuiName(std::string name) { tagGuiName = name; }
		void setTagInGuiStereo(bool stereo) { tagInGuiStereo = stereo; }
        
        std::string getTagInGuiName() const { return tagGuiName; }
		bool getTagInGuiStereo() const { return tagInGuiStereo; }
        
        void rocketGUI(const char * pathToRML);
        
        virtual void inUnlock();
        
        virtual std::shared_ptr<azure::Eventable> get_eventable() const {
            return nullptr;
        }
        
    protected:
        bool getTagInGuiMember() const {
            return tagInGuiMember;
        }
        
        int getTagInGuiIndex() const {
            return tagInGuiIndex;
        }
        
        tagInGui * tagGui = nullptr;
        bool tagInGuiUseExposure = true;
        
        cl::CommandQueue * queue = nullptr;
        
    private:
        std::string tagGuiName = "untitled";
        std::string rml = "";
        bool tagInGuiMember = false;
        int tagInGuiIndex = -1;
		bool tagInGuiStereo = false;
    };
    
}

#endif
