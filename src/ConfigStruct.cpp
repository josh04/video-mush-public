//
//  ConfigStruct.cpp
//  video-mush
//
//  Created by Josh McNamee on 19/03/2017.
//
//

#include "ConfigStruct.hpp"

namespace mush {

const char * processEngineToString(processEngine e, genericProcess g, generatorProcess f) {
    const char * r = nullptr;
    
    switch (e) {
        case processEngine::tonemapCompress:
        r = "Tonemap";
        break;
        
        case processEngine::slic:
        r = "SLIC";
        break;
        
        case processEngine::waveform:
        r = "Waveform";
        break;
        
        case processEngine::sand:
        {
            switch (f) {
                case generatorProcess::sand:
                r = "Sand";
                break;
                
                case generatorProcess::text:
                r = "Text";
                break;
                
                case generatorProcess::sphere:
                r = "360";
                break;
                
                case generatorProcess::oculusVideo:
                r = "Oculus 360";
                break;
                
                case generatorProcess::oculusDraw:
                r = "Oculus Draw";
                break;
                
                case generatorProcess::motionReprojection:
                r = "Motion Reprojection";
                break;
                
                case generatorProcess::anaglyph:
                r = "Anaglyph";
                break;
                
                case generatorProcess::motionGenerator:
                r = "Motion Generator";
                break;
                
                case generatorProcess::sbsPack:
                r = "Side-by-side packer";
                break;
                
                case generatorProcess::sbsUnpack:
                r = "Side-by-side unpacker";
                break;

				case generatorProcess::mse:
				r = "MSE";
				break;
                
            }
        }
        break;
        
        case processEngine::fixedBitDepth:
        r = "Fixed Bit Depth";
        break;
        
        case processEngine::nuHDR:
        r = "nuHDR";
        break;
        
        case processEngine::generic:
        {
            switch (g) {
                case genericProcess::barcode:
                r = "Barcode";
                break;
                
                case genericProcess::displaceChannel:
                r = "Barcode";
                break;
                
                case genericProcess::rotateImage:
                r = "Barcode";
                break;
                
                case genericProcess::falseColour:
                r = "Barcode";
                break;
                
                case genericProcess::bt709luminance:
                r = "Barcode";
                break;
                
                case genericProcess::tonemap:
                r = "Barcode";
                break;
                
                case genericProcess::colourBilateral:
                r = "Barcode";
                break;
                
                case genericProcess::laplace:
                r = "Barcode";
                break;
                
                case genericProcess::debayer:
                r = "Barcode";
                break;
                
                case genericProcess::tape:
                r = "Barcode";
                break;
                
                case genericProcess::fisheye2equirectangular:
                r = "Barcode";
                break;
                
                case genericProcess::sobel:
                r = "Barcode";
                break;

				case genericProcess::videoAverage:
				r = "Video Average";
				break;
            }
        }
        break;
        
        case processEngine::debayer:
        r = "Debayer";
        break;
        
        case processEngine::trayrace:
        r = "Remote Render";
        break;
        
        case processEngine::par:
        r = "PAR";
        break;
        
        case processEngine::amd:
        r = "RadeonRays";
        break;
        
        case processEngine::oculusDraw:
        r = "Oculus Direct Draw";
        break;
        
        case processEngine::motionExplorer:
        r = "Motion Explorer";
        break;
        
        case processEngine::metrics:
        r = "Metrics";
        break;
        
        case processEngine::none:
        r = "N/A";
        break;
        
    }
    
    return r;
}

}

