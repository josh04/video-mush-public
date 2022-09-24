//
//  inputConfig.hpp
//  mush-core
//
//  Created by Josh McNamee on 05/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#ifndef inputConfig_h
#define inputConfig_h

#include "outputConfig.hpp"

namespace mush {
    
    enum class rawCameraType : int {
        mk2 = 0,
        mk3 = 1
    };
        
    enum class bmModes {
        m720p60,
        m1080p24,
        m1080p25,
        m1080p30
    };
    
    /* external input pixel format
     char_4channel is BGRA,
     other two are RGBA */
    enum class input_pix_fmt {
        char_4channel,
        half_4channel,
        float_4channel
    };
    
    /* choice of file input
     for folderInput.
     detectFiletype is default */
    enum class filetype {
        pfmFiletype,
        rawFiletype,
        exrFiletype,
        mergeTiffFiletype,
        tiffFiletype,
        detectFiletype
    };
        
    /* choice of input format.
     stdin not implemented */
    enum class inputEngine {
        folderInput,
        stdinInput,
        videoInput,
        rgb16bitInput,
        externalInput,
        flareInput,
        fastEXRInput,
        udpInput,
        flare10Input,
        testCardInput,
        jpegInput,
        canonInput,
        yuv10bitInput,
        singleEXRInput,
        mlvRawInput,
        noInput
    };
        
    const char * inputEngineToString(inputEngine e);
        
    namespace core {
        struct inputConfigStruct {
        public:
            void defaults() {
                inputEngine = inputEngine::testCardInput;
                
                inputPath = "e:/resources/MorganLovers";
                filetype = filetype::detectFiletype;
                
                inputBuffers = 2;
                exposures = 1;
                
                inputWidth = 1280;
                inputHeight = 720;
                inputSize = 1;
                
                noflip = false;
                
                
                input_pix_fmt = input_pix_fmt::float_4channel;
                
    #ifdef __APPLE__
                //flareCOMPort = "/dev/tty.usbserial-FTVICOFS";
                flareCOMPort = "/dev/tty.usbserial-FTVIA0H4";
                
                testCardPath = "TestCard.exr";
    #endif
    #ifdef _WIN32
                flareCOMPort = "COM3";
                testCardPath = "resources/TestCard.exr";
    #endif
                loopFrames = false;
                bmMode = bmModes::m1080p30;
                func = transfer::linear;
                
                dualISO = false;
                
                whitePoint[0] = 1.392498f;
                whitePoint[1] = 1.0f;
                whitePoint[2] = 2.375114f;
                whitePoint[3] = 1.0f;
                blackPoint = 0;
                dual_iso_comp_factor = 3.0f;
                camera_type = rawCameraType::mk3;
                frame_skip = 0;
                raw_clamp = 1.0f;
                
                resize = false;
                resize_width = 1280;
                resize_height = 720;
                gammacorrect = 1.0f;
                darken = 0.0f;
                
                lock_fps = false;
                fps = 25.0f;
                
                secondInputPath = "";
                thirdInputPath = "";
                fourthInputPath = "";
            }
            
            inputEngine inputEngine;
            
            const char * inputPath = "";
            filetype filetype;
            int inputBuffers; // number of input buffers. no idea why you'd change it.
            int exposures; // a sub-property of mergeEngine. make sure they match!
            
            unsigned int inputWidth; // width. will be set automatically if possible
            unsigned int inputHeight; // height, ditto
            unsigned int inputSize; // size of pixel in ints
            
            bool noflip; // if your two-canon system is the right way up
            float legacyIsoArray[4]; // iso array. ascending!
            
            const char * flareCOMPort; // COM port for the flare
            
            const char * testCardPath;
            const char * resourceDir;
            
            input_pix_fmt input_pix_fmt;
            
            bool doubleHeightFrame = false; // inputting a double height [REDACTED] frame? who cares!
            
            //bool testMode = false; // Make the preprocessor spew out the last frame if the new frame isn't timely, so you can test real-time effects with a slow frame grabber;
            
            bool loopFrames;
            
            bmModes bmMode;
            
            transfer func;
            
            float whitePoint[4];
            int blackPoint;
            
            bool dualISO;
            
            float dual_iso_comp_factor;
            rawCameraType camera_type;
            
            unsigned int frame_skip;
            float raw_clamp;
            
            bool resize;
            unsigned int resize_width;
            unsigned int resize_height;
            float gammacorrect; // gamma to correct for
            float darken; // exposure modifier
            
            bool lock_fps;
            float fps;
            
			const char * secondInputPath;
			const char * thirdInputPath;
			const char * fourthInputPath;
        };
    }
}

#endif /* inputConfig_h */
