//
//  dummyEventable.hpp
//  mush-core
//
//  Created by Josh McNamee on 21/03/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#ifndef dummyEventable_h
#define dummyEventable_h

#include <azure/eventable.hpp>

namespace mush {
    namespace gui {
        class dummy_eventable : public azure::Eventable {
        public:
			dummy_eventable();
            ~dummy_eventable();
            
            void set_eventables(std::vector<std::shared_ptr<azure::Eventable>> evs);
            
            bool event(std::shared_ptr<azure::Event> event) override;
            
            private:
            std::vector<std::shared_ptr<azure::Eventable>> _evs;
        };
    }
}

#endif /* dummyEventable_h */
