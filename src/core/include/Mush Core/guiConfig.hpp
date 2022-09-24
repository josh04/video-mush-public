//
//  guiConfig.hpp
//  mush-core
//
//  Created by Josh McNamee on 08/01/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#ifndef guiConfig_h
#define guiConfig_h

namespace mush {
    
    namespace core {
    struct guiConfig {
    public:
        void defaults() {
            show_gui = false;
            sim2preview = false;
            
            subScreenRows = 8;
            
#ifdef __APPLE__
            exrDir = "./Pictures/EXR/";
#endif
#ifdef _WIN32
            exrDir = ".\\EXR\\";
#endif
			fullscreen = false;
        }
        
        bool show_gui; // show the gui?
        
        bool sim2preview; // enables sim2preview with show_gui. maybe. definitely with cameraInput
        
        unsigned int subScreenRows;
        
        const char * exrDir;

		bool fullscreen;
    };
    }
}

#endif /* guiConfig_h */
