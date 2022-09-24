//
//  dummyEventable.cpp
//  mush-core
//
//  Created by Josh McNamee on 21/03/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#include "dummyEventable.hpp"

namespace mush {
    namespace gui {
        
		dummy_eventable::dummy_eventable() : _evs({}) {
            
        }
        
		dummy_eventable::~dummy_eventable() {
            
        }
        
        void dummy_eventable::set_eventables(std::vector<std::shared_ptr<azure::Eventable>> evs) {
            _evs = evs;
        }
        
        bool dummy_eventable::event(std::shared_ptr<azure::Event> event) {
            bool ret = false;
            for (auto ev : _evs) {
                if (ev != nullptr) {
                    ret = ev->event(event) || ret;
                }
            }
            return ret;
        }
        
    }
}
