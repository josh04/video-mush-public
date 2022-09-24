//
//  inputConfig.cpp
//  mush-core
//
//  Created by Josh McNamee on 18/03/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#include "inputConfig.hpp"

namespace mush {

const char * inputEngineToString(inputEngine e) {
    const char * r = nullptr;
    switch (e) {
        case inputEngine::folderInput:
            r = "Frames";
        break;
        case inputEngine::stdinInput:
        r = "stdin";
        break;
        case inputEngine::videoInput:
        r = "Video";
        break;
        case inputEngine::rgb16bitInput:
        r = "RGB 16-bit RAW";
        break;
        case inputEngine::externalInput:
        r = "Ext.";
        break;
        case inputEngine::flareInput:
        r = "Old Flare SDI";
        break;
        case inputEngine::fastEXRInput:
        r = "Multi-threaded EXR";
        break;
        case inputEngine::udpInput:
        r = "UDP";
        break;
        case inputEngine::flare10Input:
        r = "SDI";
        break;
        case inputEngine::testCardInput:
        r = "Test Card";
        break;
        case inputEngine::jpegInput:
        r = "JPEG Frames";
        break;
        case inputEngine::canonInput:
        r = "Canon";
        break;
        case inputEngine::yuv10bitInput:
        r = "YUV 10-bit RAW";
        break;
        case inputEngine::singleEXRInput:
        r = "Still Frame";
        break;
        case inputEngine::mlvRawInput:
        r = "ML RAW";
        break;
        case inputEngine::noInput:
        r = "N/A";
        break;
    }
    return r;
}
}
