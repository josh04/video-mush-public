//
//  genericProcessor.cpp
//  video-mush
//
//  Created by Josh McNamee on 19/01/2017.
//
//

#include <Mush Core/SetThreadName.hpp>
#include <Mush Core/fixedExposureProcess.hpp>
#include <Mush Core/psnrProcess.hpp>

#include "ConfigStruct.hpp"

#include <Mush Core/timerWrapper.hpp>

#include <Mush Core/imageProcessor.hpp>
#include <Mush Core/imageProcess.hpp>
#include "bt709luminanceProcess.hpp"
#include "barcodeProcess.hpp"
#include "displaceChannel.hpp"
#include "rotateImage.hpp"
#include "falseColourProcess.hpp"
#include "bt709luminanceProcess.hpp"
#include "tonemapProcess.hpp"
#include "colourBilateralProcess.hpp"
#include "laplaceProcess.hpp"
#include "debayerProcess.hpp"
#include "tapeProcess.hpp"
#include "textDrawProcess.hpp"
#include <Mush Core/fisheye2EquirectangularProcess.hpp>
#include "sobelProcess.hpp"
#include "videoAverageProcess.hpp"
#include "mseProcess.hpp"

#include "genericProcessor.hpp"

namespace mush {
    
    genericProcessor::genericProcessor(genericProcess p) : imageProcessor(), _process(p) {
        
    }
    
    genericProcessor::~genericProcessor() {
        
    }
    
    void genericProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
        //assert(buffers.size() == 1);
        imageBuf = castToImage(buffers.begin()[0]);

        switch (_process) {
            default:
            case genericProcess::barcode:
                _generic = make_shared<barcodeProcess>();
                _generic->setTagInGuiName("Barcode");
                break;
            case genericProcess::displaceChannel:
                _generic = make_shared<displaceChannel>();
                _generic->setTagInGuiName("Chroma Shuffle");
                break;
            case genericProcess::rotateImage:
                _generic = make_shared<rotateImage>();
                _generic->setTagInGuiName("Rotate Image");
                break;
            case genericProcess::falseColour:
                _generic = make_shared<falseColourProcess>();
                _generic->setTagInGuiName("False Colour");
                break;
            case genericProcess::bt709luminance:
                _generic = make_shared<bt709luminanceProcess>();
                _generic->setTagInGuiName("BT. 709 Luminance");
                break;
            case genericProcess::tonemap:
                _generic = make_shared<tonemapProcess>();
                _generic->setTagInGuiName("Sigmoid Tonemap");
                break;
            case genericProcess::colourBilateral:
                _generic = make_shared<colourBilateralProcess>(0.4, 0.2, 5);
                _generic->setTagInGuiName("Bilateral Filter");
                break;
            case genericProcess::laplace:
                _generic = make_shared<laplaceProcess>();
                _generic->setTagInGuiName("Laplacian Edge Detection");
                break;
            case genericProcess::debayer:
                _generic = make_shared<debayerProcess>();
                _generic->setTagInGuiName("Simple Debayer");
                break;
            case genericProcess::tape:
                _generic = make_shared<tapeProcess>();
                _generic->setTagInGuiName("Tape Shift");
                break;
            case genericProcess::fisheye2equirectangular:
                //auto fish = make_shared<fisheye2EquirectangularProcess>(185.0f);
                //videoMushAddEventHandler(fish);
                _generic = make_shared<fisheye2EquirectangularProcess>(185.0f, 0.497f, 0.52f);
                _generic->setTagInGuiName("Fish Eye");
                break;
            case genericProcess::sobel:
                _generic = make_shared<sobelProcess>();
                _generic->setTagInGuiName("Sobel Filter");
                break;
			case genericProcess::videoAverage:
				_generic = make_shared<videoAverageProcess>();
				_generic->setTagInGuiName("Video Average");
				break;
        }
        _generic->init(context, imageBuf);
        
        stepper = std::make_shared<mush::frameStepper>();
        
        //_psnr = std::make_shared<mush::psnrProcess>(psnrProcess::type::linear, 1.0f);
        //_psnr->init(context, { _generic, buffers.begin()[0] });
        _copy2 = std::make_shared<mush::fixedExposureProcess>(0.0f);
        _copy2->init(context, _generic);
        
        _guiBuffers.push_back(_generic);
        //_guiBuffers.push_back(_psnr);
        
        _timer = std::unique_ptr<timerWrapper>(new timerWrapper("Timer"));
        _timer->register_node(stepper, "Step");
        _timer->register_node(_generic, "Generic");
        //_timer->register_metric(_psnr, "PSNR");
        _timer->register_node(_copy2, "Copy 2");
    }
    
    const std::vector<std::shared_ptr<mush::ringBuffer>> genericProcessor::getBuffers() const {
        return { _copy2 };
    }
    
    std::vector<std::shared_ptr<mush::guiAccessible>> genericProcessor::getGuiBuffers() {
        const std::vector<std::shared_ptr<mush::guiAccessible>> buffs = _guiBuffers;
        _guiBuffers.clear();
        return buffs;
    }
    
    void genericProcessor::process() {
        /*stepper->process();
         _copy->process();
         _generic->process();*/
        
        _timer->process(stepper);
        _timer->process(_generic);
        //_timer->process_metric(_psnr);
        _timer->process(_copy2);
        
        _timer->print_metered_report();
    }
    
    void genericProcessor::go() {
        SetThreadName("_generic");
        while (imageBuf->good()) {
            process();
        }
        
        _timer->print_final_report();
        
        _generic->release();
        //_psnr->release();
        _copy2->release();
    }
    
    std::vector<std::shared_ptr<mush::frameStepper>> genericProcessor::getFrameSteppers() const {
        return {stepper};
    }

}
