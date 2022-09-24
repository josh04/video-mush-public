#ifndef CONFIGSTRUCT_HPP
#define CONFIGSTRUCT_HPP

#include <string>
#include <vector>

#include <Mush Core/inputConfig.hpp>
#include <Mush Core/outputConfig.hpp>
#include <Mush Core/metricConfig.hpp>
#include <Mush Core/guiConfig.hpp>
#include <Mush Core/cameraConfig.hpp>
#ifdef __APPLE__
#include <RadeonLibrary/radeonConfig.hpp>
#include <ParFramework/parConfig.hpp>
#else
#include <App/Mush/radeonConfig.hpp>
#include <PARtner/parConfig.hpp>
#endif

namespace mush {
    class imageBuffer;

/*
 
 ConfigStruct.hpp
 
 This file defines the settings struct which programs hdrControl.
 Not every path through the program uses every setting.
 
 */


    
    enum class genericProcess {
        barcode,
        displaceChannel,
        rotateImage,
        falseColour,
        bt709luminance,
        tonemap,
        colourBilateral,
        laplace,
        debayer,
        tape,
		fisheye2equirectangular,
        sobel,
		videoAverage
    };
    
    enum class generatorProcess {
        sand,
        text,
        sphere,
		oculusVideo,
		oculusDraw,
		motionReprojection,
        anaglyph,
        motionGenerator,
        sbsPack,
        sbsUnpack,
		mse,
		raster
    };
    
    // built in processEngines, for use with builtInProcessor
    enum class processEngine {
        tonemapCompress,
        slic,
        waveform,
        sand,
        fixedBitDepth,
        nuHDR,
        generic,
        debayer,
		trayrace,
        par,
        amd,
		oculusDraw,
		motionExplorer,
		metrics,

		none = 999
    };
    
    const char * processEngineToString(processEngine e, genericProcess g, generatorProcess f);
    
    enum class waveformMode {
        luma,
        rgb,
        r,
        g,
        b
    };

	enum class bilateralMode {
		bilateral,
		bilateralOpt,
		bilinear,
		off
	};
    
    enum class fbdOutputs {
        decoded,
        chromaSwap,
        banding,
        falseColour,
        switcher
    };

	enum class oculus_draw_source {
		par,
		amd,
		direct
	};

	class config {
	public:
		void defaults() {
//			cltype = CL_DEVICE_TYPE_GPU;

			processEngine = processEngine::none;

#ifdef __APPLE__
			resourceDir = "./";
#endif
#ifdef _WIN32
			resourceDir = "./";
#endif
			
			openclCPU = false;

			outputConfig.defaults();
            
			inputConfig.defaults();
			inputConfig.resourceDir = resourceDir;
            
            gui.defaults();
            
            slicConfig.defaults();
            waveformConfig.defaults();

			func = transfer::g8;
			genericChoice = genericProcess::barcode;

			demoMode = false;
			fbd_output = fbdOutputs::switcher;
			catch_exceptions = false;

			trayraceConfig.defaults();
            generatorConfig.defaults();
            parConfig.defaults();
			radeonConfig.defaults();

			metric_path = "";

			oculusConfig.defaults();
			cameraConfig.defaults();
			motionExplorer.defaults();
		}

		processEngine processEngine;
		core::inputConfigStruct inputConfig;
		core::outputConfigStruct outputConfig;
		core::guiConfig gui;

//		cl_device_type cltype;
        
		float isoArray[4]; // iso array. ascending!

		const char * resourceDir;

		transfer func; // for tf testing stuff
		genericProcess genericChoice;

		bool catch_exceptions;
		bool openclCPU;

		bool demoMode;

		fbdOutputs fbd_output;
        
        struct slicConfigStruct {
        public:
            void defaults() {
                slicDrawBorders = false;
                slicDrawCenters = false;
                slicFillCells = true;
                slicDrawUniqueness = false;
                slicDrawDistribution = false;
                slicDrawSaliency = false;
            }
        bool slicDrawBorders;
        bool slicDrawCenters;
        bool slicFillCells;
        bool slicDrawUniqueness;
        bool slicDrawDistribution;
        bool slicDrawSaliency;
        } slicConfig;
        
        struct waveformConfigStruct {
        public:
            void defaults() {
                waveformMode = waveformMode::luma;
            }
            waveformMode waveformMode;
        } waveformConfig;
        
		struct trayraceConfigStruct {
		public:
			void defaults() {
				viewportPath = "";
				depthPath = "";
				normalsPath = "";
				motionPath = "";
			}
			
			const char * viewportPath;
			const char * depthPath;
			const char * normalsPath;
			const char * motionPath;


		} trayraceConfig;
        
        struct generatorProcessStruct {
            void defaults() {
                type = generatorProcess::sand;
                text_output_string = "Once upon a time...";
                
                bg_colour[0] = 1.0f;
                bg_colour[1] = 0.5f;
                bg_colour[2] = 0.5f;
                bg_colour[3] = 1.0f;
                
                text_colour[0] = 0.9f;
                text_colour[1] = 0.9f;
                text_colour[2] = 0.9f;
                text_colour[3] = 1.0f;
            }
            generatorProcess type;
            const char * text_output_string;
            
            float bg_colour[4];
            float text_colour[4];
        } generatorConfig;
        
        parConfigStruct parConfig;

		radeonConfig radeonConfig;

		const char * metric_path;

		struct oculusConfigStruct {
			void defaults() {
				source = oculus_draw_source::par;
			}
			oculus_draw_source source;
		} oculusConfig;

		core::cameraConfigStruct cameraConfig;

		struct motionExplorerStruct {
			void defaults() {
				follow_stereo = false;
				spherical = false;
			}

			bool follow_stereo;
			bool spherical;
		};

		motionExplorerStruct motionExplorer;
	};
}

#endif
