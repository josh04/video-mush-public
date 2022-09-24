//
//  guiEventHandler.cpp
//  space
//
//  Created by Josh McNamee on 07/10/2015.
//  Copyright © 2015 josh04. All rights reserved.
//


#include "imguiEventHandler.hpp"

bool imguiEventHandler::event(std::shared_ptr<azure::Event> event) {
    if (event->isType("keyDown")) {
        _gui->keyChange(event->getAttribute<azure::Key>("key"), true);
    } else if (event->isType("keyUp")) {
        _gui->keyChange(event->getAttribute<azure::Key>("key"), false);
    } else if (event->isType("mouseDown")) {
        _gui->mouseDown(event->getAttribute<azure::MouseButton>("button"));
    } else if (event->isType("mouseUp")) {
        _gui->mouseUp(event->getAttribute<azure::MouseButton>("button"));
    } else if (event->isType("mouseScroll")) {
        int s = event->getAttribute<int>("y"); // magic number
        _gui->mouseScroll(s);
        return true;
    } else if (event->isType("mouseMove")) {
        int m_x = event->getAttribute<int>("x");
        int m_y = event->getAttribute<int>("y");
        _gui->mouseMove(m_x, m_y);
    } else if (event->isType("textEntry")) {
        auto text = event->getAttribute<std::string>("text");
        _gui->textEntry(text);
    } else if (event->isType("windowResize")) {
        
        int s_width = event->getAttribute<int>("x");
        int s_height = event->getAttribute<int>("y");
        
        _gui->resize(1280, 720, s_width, s_height);
    }
    
    return false;
}