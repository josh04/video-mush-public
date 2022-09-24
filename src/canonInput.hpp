//
//  canonInput.hpp
//  media-encoder
//
//  Created by Josh McNamee on 02/07/2014.
//  Copyright (c) 2014 video-mush. All rights reserved.
//

#ifndef media_encoder_canonInput_hpp
#define media_encoder_canonInput_hpp

#define PIXELFORMATFFF bmdFormat8BitYUV //mmmm YUV eh UYUV iirc aaaargh

#include <fstream>
#include <boost/timer/timer.hpp>

#include <Mush Core/opencl.hpp>

#include "videoframe.hpp"
#include "canon5d.hpp"
#include <Mush Core/frameGrabber.hpp>

#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#include "DeckLink-Win32/DeckLinkAPI.h"
#else
#include "DeckLink/DeckLinkAPI.h"
#endif

extern void SetThreadName(char*);

class hdrCanonInput : public mush::frameGrabber {
public:
	hdrCanonInput()
    : mush::frameGrabber(mush::inputEngine::canonInput) {
		
	}
	
	~hdrCanonInput() {
		stop();
	}
	
	
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		this->context = context;
		_width = config.inputWidth;
		_height = config.inputHeight;
		_size = 2;
		
		if (_width * _height > 0) {
			for (int i = 0; i < config.inputBuffers*config.exposures; i++) {
				addItem(context->hostWriteBuffer(_width * _height * _size)); // hopefully 10-bit up in this?
			}
		}
		
		// DeckLink Setup
#ifdef _WIN32
		CoInitialize(nullptr);
		HRESULT rul = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**) &deckLinkIterator);
		if (deckLinkIterator == NULL) {
#else
            if (!(deckLinkIterator = CreateDeckLinkIteratorInstance())) {
#endif
                std::cerr << "This application requires the DeckLink drivers installed." << std::endl;
                throw std::exception();
            }
            
            bool success = false;
            while (!success) {
                if (deckLinkIterator->Next(&deckLinkAlpha) != S_OK) {
                    std::cerr << "No valid DeckLink PCI cards found." << std::endl;
                    throw std::exception();
                }
                
                if (deckLinkAlpha->QueryInterface(IID_IDeckLinkInput, (void **) &deckLinkInputAlpha) != S_OK) {
                    std::cerr << "Something went wrong acquiring the interface." << std::endl;
                    throw std::exception();
                } else {
                    IDeckLinkConfiguration * conf;
                    deckLinkInputAlpha->QueryInterface(IID_IDeckLinkConfiguration, (void **) &conf);
                    int64_t inp;
                    conf->GetInt(bmdDeckLinkConfigVideoInputConnection, &inp);
                    if (inp != bmdVideoConnectionHDMI) {
                        putLog("DeckLink card not HDMI.");
                    } else {
                        success = true;
                    }
                }
            }
            
            canonDelegate = new Canon5DCaptureDelegate();
            
            canonDelegate->frames = &list;
            canonDelegate->framesMutex = &listMutex;
            canonDelegate->framesCond = &listCond;
            canonDelegate->id = 0;
            deckLinkInputAlpha->SetCallback((IDeckLinkInputCallback *)canonDelegate);
            
            BMDDisplayMode dispmode;
            //if (_height == 720) {
            //    dispmode = bmdModeHD720p60;
            //} else {
                dispmode = bmdModeHD1080i5994;
            //}
			
            if ((result = deckLinkInputAlpha->EnableVideoInput(dispmode, PIXELFORMATFFF, bmdVideoInputFlagDefault)) != S_OK) {
                putLog("Failed to enable video input. Is another application using the card?");
                throw std::exception();
            }
            
        }
        
        void gather() {
            SetThreadName("canonCapture");
            int64_t frames = 0;
            
            putLog("Beginning capture ...");
            if ((result = deckLinkInputAlpha->StartStreams()) != S_OK) {
                throw std::exception();
            }
            
            while(true) { //deckLinkInputAlpha->Okay()) { // this should be able to stop itself yknow
                VideoFrame * videoFrame;
                IDeckLinkVideoInputFrame * videoInputFrame;
                void * ptr;
                
                {
                    std::unique_lock<std::mutex> lock(listMutex);
                    listCond.wait(lock, [&](){return !list.empty(); });
                    
                    videoFrame = list.front();
                    list.pop_front();
                }
                
                videoInputFrame = videoFrame->frame;

				auto buf = inLock();
                if (buf == nullptr) {
                    return;
                }
				unsigned char * ptr2 = (unsigned char *)buf.get_pointer();
                videoInputFrame->GetBytes(&ptr);
                memcpy(ptr2, ptr, _width*_height*_size);
                
                inUnlock();
                
                videoInputFrame->Release();
                delete videoFrame;
                
                
                ++frames;
            }
            
        }
        
        void stop() {
            putLog("Ending capture ...");
            deckLinkInputAlpha->StopStreams();
            
            // Empty the frame queue
            {
                boost::lock_guard<std::mutex> lock(listMutex);
                while (!list.empty()) {
                    VideoFrame * videoFrame;
                    videoFrame = list.front();
                    list.pop_front();
                    videoFrame->frame->Release();
                    delete videoFrame;
                }
            }
            
            if (deckLinkInputAlpha != NULL) {
                deckLinkInputAlpha->Release();
                deckLinkInputAlpha = NULL;
            }
            
            if (deckLinkAlpha != NULL) {
                deckLinkAlpha->Release();
                deckLinkAlpha = NULL;
            }
            
            if (deckLinkIterator != NULL) {
                deckLinkIterator->Release();
                deckLinkIterator = NULL;
            }
        }
        
	protected:
    private:
        
        std::shared_ptr<mush::opencl> context;
        Canon5DCaptureDelegate * canonDelegate;
        
        IDeckLinkIterator * deckLinkIterator;
        IDeckLink * deckLinkAlpha;
        IDeckLinkInput * deckLinkInputAlpha;
        HRESULT result;
        
        std::mutex listMutex;
        std::condition_variable listCond;
        std::list<VideoFrame *> list;
        
    };


#endif
