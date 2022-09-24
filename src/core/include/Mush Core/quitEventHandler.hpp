//
//  quitEventHandler.hpp
//  video-mush
//
//  Created by Josh McNamee on 21/03/2015.
//
//

#ifndef video_mush_quitEventHandler_hpp
#define video_mush_quitEventHandler_hpp

#include <azure/Eventable.hpp>
#include <azure/Event.hpp>
#include <azure/Eventkey.hpp>

namespace mush {
    class quitEventHandler : public azure::Eventable {
    public:
        quitEventHandler() {}
        ~quitEventHandler() {}
        
        bool event(std::shared_ptr<azure::Event> event) {
            if (event->isType("quit")) {
                quit = true;
                return false;
            }
            return false;
        }
        
        bool getQuit() {
            return quit;
        }
    private:
        bool quit = false;
    };
}

#endif
