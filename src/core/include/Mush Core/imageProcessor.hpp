//
//  imageProcessor.hpp
//  video-mush
//
//  Created by Josh McNamee on 12/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_imageProcessor_hpp
#define media_encoder_imageProcessor_hpp

#include <memory>
#include <vector>

namespace mush {
    class imageBuffer;
    class frameStepper;
    class guiAccessible;
    class ringBuffer;
    class opencl;
}

namespace azure {
    class Eventable;
}

#include "processNode.hpp"
#include "initNode.hpp"

namespace mush {
    class imageProcessor : public processNode, public initNode /*, public azure::Eventable */{
    public:
        imageProcessor() : processNode() {
            
        }
        
        ~imageProcessor() {
            
        }
        
        virtual void process() = 0;
        
        virtual void go() = 0;
        
        void startThread() {
            go();
        }
        
        virtual const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const = 0;
        
        virtual std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() = 0;
        
        virtual std::vector<std::shared_ptr<mush::frameStepper>> getFrameSteppers() const {
            return std::vector<std::shared_ptr<mush::frameStepper>>();
        }
        
        virtual std::vector<std::shared_ptr<azure::Eventable>> getEventables() const {
            return {};
        }
        /*
         bool event(std::shared_ptr<azure::Event> event) {
         if (event->isType("quit")) {
         engine->removeEventHandler(shared_from_this());
         azure::Events::Push(std::unique_ptr<azure::Event>(new azure::QuitEvent()));
         return true;
         }
         return false;
         }*/
        
    private:
    };
}

#endif
