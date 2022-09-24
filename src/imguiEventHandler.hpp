//
//  guiEventHandler.hpp
//  space
//
//  Created by Josh McNamee on 07/10/2015.
//  Copyright © 2015 josh04. All rights reserved.
//

#ifndef guiEventHandler_hpp
#define guiEventHandler_hpp

#include <azure/Eventable.hpp>
#include <azure/Event.hpp>
#include <azure/Eventkey.hpp>
#include <azure/Events.hpp>

#include <memory>
#include "imguiDrawer.hpp"

class imguiEventHandler : public azure::Eventable {
public:
    imguiEventHandler(std::shared_ptr<imguiDrawer> pl) : azure::Eventable(), _gui(pl) {
        
    }
    
    ~imguiEventHandler() {
        
    }
    
    bool event(std::shared_ptr<azure::Event> event);
private:
    std::shared_ptr<imguiDrawer> _gui = nullptr;
};

#endif /* guiEventHandler_hpp */
