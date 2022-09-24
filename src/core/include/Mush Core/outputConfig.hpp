//
//  outputConfig.hpp
//  mush-core
//
//  Created by Josh McNamee on 05/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#ifndef outputConfig_h
#define outputConfig_h

namespace mush {
    
    /* output engine */
    enum class outputEngine {
        //		screenOutput,
        libavformatOutput,
        noOutput
    };
    
    /* chroma subsample for
     x264 */
    enum class chromaSubsample {
        yuv420p,
        yuv422p,
        yuv444p
    };
    
    enum class transfer {
        pq,
        g8,
        srgb,
        linear,
        logc,
        gamma,
        rec709
    };
    
    /* packet maker */
    enum class encodeEngine {
        x264,
        x265,
        nvenc,
        exr,
        yuvRaw,
        vpx,
        prores,
        gif,
        none
    };
    
    const char * outputEngineToString(outputEngine e);
    const char * encodeEngineToString(encodeEngine e);
    const char * transferToString(transfer e);
    
    namespace core {
        struct outputConfigStruct {
        public:
            void defaults() {
                outputEngine = outputEngine::noOutput;
                
                encodeEngine = encodeEngine::none;
                
                bitrate = 0;
                outputBuffers = 2;
                outputName = "output";
        #ifdef __APPLE__
                outputPath = "/Users/visualisation/Movies";
        #endif
        #ifdef _WIN32
                outputPath = "../output/";
        #endif
                h264Profile = "high";
                h264Preset = "faster";
                height = 720;
                width = 1280;
                fps = 29.97f;
                
                yuvBitDepth = 10;
                yuvMax = 1.0f;
                pqLegacy = false;
                
                func = transfer::g8;
                zerolatency = false;
                crf = 28;
                
                overrideSize = false;
                
                use_auto_decode = false;
                
                liveStream.defaults();
                
                frame_skip_interval = 0;
                
                chromaSubsample = chromaSubsample::yuv420p;

				count_from = 0;
                
                exr_timestamp = false;
            }
            
            outputEngine outputEngine;
            
            encodeEngine encodeEngine;
            
            float fps; // fps for x264 and vlc
            
            unsigned int bitrate;
            unsigned int outputBuffers; // number of output buffers. about as useful.
            const char * outputPath;
            const char * outputName;
            
            const char * h264Profile;
            const char * h264Preset;
            
            unsigned int height; // size of output. double height
            unsigned int width; // for [REDACTED], regular for tonemap
            
            int yuvBitDepth;
            float yuvMax;
            bool yuvDryRun;
            bool pqLegacy;
            
            transfer func;
            
            
            bool zerolatency; // ???
            
            unsigned int crf;
            
            bool overrideSize;
            
            bool use_auto_decode;
            
            struct liveStreamStruct {
                void defaults() {
                    enabled = false;
                    webroot = "";
                    webaddress = "";
                    streamname = "";
                    
                    num_files = 5;
                    file_length = 5;
                    wrap_after = 5;
                }
                bool enabled;
                const char * webroot;
                const char * webaddress;
                const char * streamname;
                int num_files;
                int file_length;
                int wrap_after;
            } liveStream;
            
            unsigned int frame_skip_interval;
            
            chromaSubsample chromaSubsample;

			int count_from;
            
            bool exr_timestamp;
        };

    }
}

#endif /* outputConfig_h */
