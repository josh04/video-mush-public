//
//  fixedBitDepthProcessor.cpp
//  video-mush
//
//  Created by Josh McNamee on 20/01/2015.
//
//

#include <stdio.h>
#include "falseColourProcess.hpp"
#include <Mush Core/opencl.hpp>
#include "fixedBitDepthProcessor.hpp"
#include "bt709luminanceProcess.hpp"
#include "pqEncodeProcess.hpp"
#include "pqDecodeProcess.hpp"
#include "shortFalseColour.hpp"
#include "yuvDepthClampProcess.hpp"
#include "hdrProcessor.hpp"
#include <Mush Core/psnrProcess.hpp>
#include "tonemapProcess.hpp"
#include "switcherProcess.hpp"
#include <Mush Core/integerMapProcess.hpp>
#include <Mush Core/fixedExposureProcess.hpp>
#include "PTF4DecodeProcess.hpp"
#include "PTF4Process.hpp"
#include "ffmpegEncodeDecode.hpp"

#include "g8DecodeTest.hpp"
#include "pqDecodeTest.hpp"

#include "chromaSwapProcess.hpp"


extern void SetThreadName(const char * threadName);

void fixedBitDepthProcessor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
    
    if (buffers.size() < 1) {
        putLog("No buffers at encoder");
    }
    imageBuf = buffers.begin()[0];
    
    queue = context->getQueue();
    
    _out = make_shared<mush::fixedExposureProcess>(0.0f);
    
	if (func == mush::transfer::pq) {
        _pq = make_shared<mush::pqEncodeProcess>(yuvMax, pqLegacy);
        _clamp = make_shared<mush::yuvDepthClampProcess>(bitDepth, 1.0f);
        
        _pq->init(context, imageBuf);
        //_out->init(context, _pq);
        
        _clamp->init(context, _pq);
	} else if (func == mush::transfer::g8) {
		_pq = make_shared<mush::PTF4Process>(yuvMax);
		_clamp = make_shared<mush::yuvDepthClampProcess>(bitDepth, 1.0f);
		
		_pq->init(context, imageBuf);
		//_out->init(context, _pq);
		
		_clamp->init(context, _pq);
    } else {
        _clamp = make_shared<mush::yuvDepthClampProcess>(bitDepth, yuvMax);
        _out->init(context, imageBuf);
        _clamp->init(context, imageBuf);
    }
    
	if (func == mush::transfer::pq) {
        _pqD = make_shared<mush::pqDecodeProcess>(yuvMax, pqLegacy);
        _pqD->init(context, _clamp);
        
        
        
        _fc = make_shared<mush::falseColourProcess>();
        _fc->init(context, _pqD);
        
        
/*        _psnr = make_shared<mush::psnrProcess>(yuvMax);
 
        std::vector<std::shared_ptr<mush::ringBuffer>> vec3;
        vec3.push_back(imageBuf);
        vec3.push_back(_pqD);
        _psnr->init(context, vec3);*/
	} else if (func == mush::transfer::g8) {
		_gamD = make_shared<mush::PTF4DecodeProcess>(yuvMax);
		_gamD->init(context, _clamp);
        _fc = make_shared<mush::falseColourProcess>();
		_fc->init(context, _gamD);
        
	}
	
    _sfc = make_shared<mush::shortFalseColour>();
    _sfc->init(context, _clamp);
    
    _chromaSwap = make_shared<mush::chromaSwapProcess>();
    _chromaSwap->init(context, {imageBuf, _sfc});
    
    
    _switcher = make_shared<mush::switcherProcess>();
    std::initializer_list<std::shared_ptr<mush::ringBuffer>> vec4;
    if (_gamD != nullptr) {
        vec4 = {_gamD, _fc, _chromaSwap, _sfc, imageBuf};
    }
    if (_pqD != nullptr) {
        vec4 = {_pqD, _fc, _chromaSwap, _sfc, imageBuf};
    }
    
    _switcher->init(context, vec4);
    
    switch (_output) {
        case mush::fbdOutputs::decoded:
            if (_gamD != nullptr) {
                _out->init(context, _gamD);
            }
            if (_pqD != nullptr) {
                _out->init(context, _pqD);
            }
            break;
        case mush::fbdOutputs::chromaSwap:
            _out->init(context, _chromaSwap);
            break;
        case mush::fbdOutputs::banding:
            _out->init(context, _sfc);
            break;
        case mush::fbdOutputs::falseColour:
            _out->init(context, _fc);
            break;
        case mush::fbdOutputs::switcher:
            _out->init(context, _switcher);
            break;
    }
    
    _ff1 = std::make_shared<mush::ffmpegEncodeDecode>(avcodec_codec::x264, mush::transfer::rec709, 34);
    _ff1->init(context, {_out});
    /*
    _ff2 = std::make_shared<mush::ffmpegEncodeDecode>();
    _ff2->init(context, {_ff1});
    _ff3 = std::make_shared<mush::ffmpegEncodeDecode>();
    _ff3->init(context, {_ff2});
    _ff4 = std::make_shared<mush::ffmpegEncodeDecode>();
    _ff4->init(context, {_ff3});
    _ff5 = std::make_shared<mush::ffmpegEncodeDecode>();
    _ff5->init(context, {_ff4});
    */
    
    _pqD_test = std::make_shared<mush::pqDecodeTest>(yuvMax);
    _pqD_test->init(context, _pq);
    
    _profile.init();
}

void fixedBitDepthProcessor::process() {
    
    //_profile.inReadStart();
    if (_pq != nullptr) {
        _pq->process();
    }
    //_profile.inReadStop();
    
    _clamp->process();
    
    if (_pqD != nullptr) {
        _pqD->process();
//        _psnr->process();
    }
	
	if (_gamD != nullptr) {
		_gamD->process();
    }
    
    if (_sfc != nullptr) {
        _sfc->process();
    }
    
    if (_fc != nullptr) {
        _fc->process();
    }
    
    if (_chromaSwap != nullptr) {
        _chromaSwap->process();
    }
    
    _switcher->process();
    
    /*if (_switcher->good()) {
        _switcher->outLock();
        _switcher->outUnlock();
    }*/

    if (_ff1 != nullptr && !threadsSpawned) {
        ffTh.push_back(std::thread(&mush::ffmpegEncodeDecode::go, _ff1));
        /*
        ffTh.push_back(std::thread(&mush::ffmpegEncodeDecode::go, _ff2));
        ffTh.push_back(std::thread(&mush::ffmpegEncodeDecode::go, _ff3));
        ffTh.push_back(std::thread(&mush::ffmpegEncodeDecode::go, _ff4));
        ffTh.push_back(std::thread(&mush::ffmpegEncodeDecode::go, _ff5));
         */
        threadsSpawned = true;
    };
    
    _out->process();
    
    _profile.start();
    _profile.inReadStart();
    _pqD_test->process();
    _profile.inReadStop();
    
    _profile.stop();
    _profile.report();
    
}

const std::vector<std::shared_ptr<mush::ringBuffer>> fixedBitDepthProcessor::getBuffers() const {
    return {_ff1};
}

std::vector<std::shared_ptr<mush::guiAccessible>> fixedBitDepthProcessor::getGuiBuffers() {
    auto vec = std::vector<std::shared_ptr<mush::guiAccessible>>();
    vec.push_back(std::dynamic_pointer_cast<mush::guiAccessible>(imageBuf));
    if (_pq != nullptr) {
        _pq->setTagInGuiName("pq-encoded");
        //vec.push_back(_pq);
    }
    if (_clamp != nullptr) {
        _clamp->setTagInGuiName("16bit");
        vec.push_back(_clamp);
    }
    if (_pqD != nullptr) {
        _pqD->setTagInGuiName("pq-decoded");
        vec.push_back(_pqD);
	}
	if (_gamD != nullptr) {
		_gamD->setTagInGuiName("gam8-decoded");
		vec.push_back(_gamD);
    }
    if (_chromaSwap != nullptr) {
        _chromaSwap->setTagInGuiName("Chroma Swap");
        vec.push_back(_chromaSwap);
    }
    if (_sfc != nullptr) {
        _sfc->setTagInGuiName("pq-banding");
        vec.push_back(_sfc);
    }
    if (_fc != nullptr) {
        _fc->setTagInGuiName("pq-falsecolour");
        vec.push_back(_fc);
    }
    if (_switcher != nullptr) {
        _switcher->setTagInGuiName("switcher");
        vec.push_back(_switcher);
    }
    if (_ff1 != nullptr) {
        _ff1->setTagInGuiName("ff1");
        vec.push_back(_ff1);
        /*
         _ff2->setTagInGuiName("ff2");
        vec.push_back(_ff2);
        _ff3->setTagInGuiName("ff3");
        vec.push_back(_ff3);
        _ff4->setTagInGuiName("ff4");
        vec.push_back(_ff4);
        _ff5->setTagInGuiName("ff5");
        vec.push_back(_ff5);
         */
    }
    return vec;
}

void fixedBitDepthProcessor::go() {
    SetThreadName("fixedBitDepth");
    while (imageBuf->good()) {
        process();
    }
    
    _profile.finalReport();
    
    if (_pq != nullptr) {
        _pq->release();
    }
    if (_clamp != nullptr) {
        _clamp->release();
	}
	if (_pqD != nullptr) {
		_pqD->release();
	}
	if (_gamD != nullptr) {
		_gamD->release();
    }
    if (_chromaSwap != nullptr) {
        _chromaSwap->release();
    }
	
    if (_fc != nullptr) {
        _fc->release();
    }
    
    if (_sfc != nullptr) {
        _sfc->release();
    }
    
    if (_switcher != nullptr) {
        _switcher->release();
    }
    
    if (_out != nullptr) {
        _out->release();
    }
    
    if (_ff1 != nullptr) {
        for (auto &th : ffTh) {
            th.join();
        }
    }
    
}
