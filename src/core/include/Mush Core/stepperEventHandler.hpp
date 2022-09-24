//
//  stepperEventHandler.hpp
//  video-mush
//
//  Created by Josh McNamee on 21/03/2015.
//
//

#ifndef video_mush_stepperEventHandler_hpp
#define video_mush_stepperEventHandler_hpp

#include <azure/Eventable.hpp>
#include <azure/Event.hpp>
#include <azure/Eventkey.hpp>

#include "frameStepper.hpp"

namespace mush {
    class stepperEventHandler : public azure::Eventable {
    public:
        stepperEventHandler(std::vector<shared_ptr<mush::frameStepper>> steppers) : steppers(steppers) {}
        ~stepperEventHandler() {}
        
        void destroy() {
            for (auto ptr : steppers) {
                if (ptr != nullptr) {
                    ptr->release();
                }
            }
        }
        
        bool event(std::shared_ptr<azure::Event> event) {
            if (event->isType("mushStep")) {
                if (steppers.size()) {
                    steppers[step]->throw_switch();
                    step = (step+1) % steppers.size();
                }
            }
            if (event->isType("mushPause")) {
                for (auto ptr : steppers) {
                    ptr->toggle();
                }
            }
            return false;
        }
        
        bool getQuit() {
            return quit;
        }
    private:
        std::vector<shared_ptr<mush::frameStepper>> steppers;
        bool quit = false;
        int step = 0;
    };
}

#endif
