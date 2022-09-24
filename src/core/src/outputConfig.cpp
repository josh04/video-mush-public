//
//  outputConfig.cpp
//  mush-core
//
//  Created by Josh McNamee on 18/03/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#include "outputConfig.hpp"

namespace mush {
    
    const char * outputEngineToString(outputEngine e) {
        const char * r = nullptr;
        
        switch (e) {
            case outputEngine::libavformatOutput:
            r = "ffmpeg";
            break;
            case outputEngine::noOutput:
            r = "N/A";
            break;
        }
        
        return r;
    }
    
    const char * encodeEngineToString(encodeEngine e) {
        const char * r = nullptr;
        
        switch (e) {
            case encodeEngine::x264:
            r = "x264";
            break;
            case encodeEngine::x265:
            r = "x265";
            break;
            case encodeEngine::nvenc:
            r = "nvenc";
            break;
            case encodeEngine::exr:
            r = "EXR";
            break;
            case encodeEngine::yuvRaw:
            r = "YUV";
            break;
            case encodeEngine::vpx:
            r = "vpx";
            break;
            case encodeEngine::prores:
            r = "prores";
            break;
            case encodeEngine::gif:
            r = "gif";
            break;
            case encodeEngine::none:
            r = "N/A";
            break;
        }
        
        return r;
    }
    
    const char * transferToString(transfer e) {
        const char * r = nullptr;
        
        switch (e) {
            case transfer::pq:
            r = "PQ";
            break;
            case transfer::g8:
            r = "PTF4";
            break;
            case transfer::srgb:
            r = "sRGB";
            break;
            case transfer::linear:
            r = "linear";
            break;
            case transfer::logc:
            r = "logC";
            break;
            case transfer::gamma:
            r = "gamma";
            break;
            case transfer::rec709:
            r = "rec709";
            break;
        }
        
        return r;
    }
}
