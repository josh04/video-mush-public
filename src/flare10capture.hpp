#ifndef FLARECAPTURE10_HPP
#define FLARECAPTURE10_HPP

#define PIXELFORMATFF bmdFormat10BitYUV //mmmm YUV eh UYUV iirc aaaargh

#include <fstream>
#include <boost/timer/timer.hpp>

#include <Mush Core/opencl.hpp>

//#include "flarecapture.hpp"

#include "videoframe.hpp"
#include "flare2k.hpp"
#include <Mush Core/frameGrabber.hpp>
#include <Mush Core/SetThreadName.hpp>

#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#include "DeckLink-Win32/DeckLinkAPI.h"
#else
#include "DeckLink/DeckLinkAPI.h"
#endif

//extern void SetThreadName(const char *);

class hdrFlare10 : public mush::frameGrabber {
public:
	hdrFlare10()
		: mush::frameGrabber(mush::inputEngine::flare10Input) {
		setTagInGuiName("Flare SDI 10-bit Input");
	}
	
	~hdrFlare10() {
		stop();
	}
    
    void destroy() override {
        _list_disabled = true;
        listCond.notify_all();
        ringBuffer::destroy();
    }
	
	
	void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
		this->context = context;
		_size = 128;

		const char * port = config.flareCOMPort;

        bmMode = config.bmMode;
        
        
        BMDDisplayMode dispmode;
        
        switch (bmMode) {
            case mush::bmModes::m720p60:
                dispmode = bmdModeHD720p60;
                _width = 1280;
                _height = 720;
                break;
            case mush::bmModes::m1080p24:
                dispmode = bmdModeHD1080p24;
                _width = 1920;
                _height = 1080;
                break;
            case mush::bmModes::m1080p25:
                dispmode = bmdModeHD1080p25;
                _width = 1920;
                _height = 1080;
                break;
            case mush::bmModes::m1080p30:
            default:
                dispmode = bmdModeHD1080p30;
                _width = 1920;
                _height = 1080;
                break;
        }
        
        if (config.exposures < 2) {
			putLog("Nulling port");
            port = NULL;
        }
		
		if (_width % 48 > 0) {
			length = _width + (48 - (_width % 48));
		} else {
			length = _width;
		}
			
		if (_width * _height > 0) {
			for (int i = 0; i < config.inputBuffers*config.exposures; i++) {
				addItem(context->hostWriteBuffer((length/48) * _height * _size)); // hopefully 10-bit up in this?
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
            throw std::runtime_error("Flare Input: This application requires the DeckLink drivers installed.");
			
		}
		
		bool success = false;
		while (!success) {

			auto ret = deckLinkIterator->Next(&deckLinkAlpha);

			if (taken > 0){
				--taken;
				continue;
			}

			if (ret == S_FALSE) {
				throw std::runtime_error("Flare Input: No valid DeckLink PCI cards found.");
			} else if (ret == E_FAIL) {
				putLog("Flare Input: Card in use, trying another");
				//throw std::exception();
				continue;
			}

			if (deckLinkAlpha->QueryInterface(IID_IDeckLinkInput, (void **) &deckLinkInputAlpha) != S_OK) {
                throw std::runtime_error("Flare Input: Something went wrong acquiring the interface.");
			} else {
				IDeckLinkConfiguration * conf;
				deckLinkInputAlpha->QueryInterface(IID_IDeckLinkConfiguration, (void **) &conf);
				int64_t inp;
				conf->GetInt(bmdDeckLinkConfigVideoInputConnection, &inp);
				if (inp != bmdVideoConnectionSDI) {
					putLog("Flare Input: DeckLink card not SDI.");
				} else {
					success = true;
					taken++;
				}
			}
		}
		
		putLog("Initialising Flare");

//		Sleep(10);
		if (port) {
			std::stringstream strm;

			strm << "Using COM:" << port;

			std::cout << strm.str() << std::endl;
			flareDelegate = new Flare2KCaptureDelegate(_height, config.exposures, port);
		} else {
			flareDelegate = new Flare2KCaptureDelegate(_height, config.exposures);
		}
		
		flareDelegate->frames = &list;
		flareDelegate->framesMutex = &listMutex;
		flareDelegate->framesCond = &listCond;
		flareDelegate->id = 0;
		deckLinkInputAlpha->SetCallback((IDeckLinkInputCallback *)flareDelegate);
		
			
		if ((result = deckLinkInputAlpha->EnableVideoInput(dispmode, PIXELFORMATFF, bmdVideoInputFlagDefault)) != S_OK) {
            throw std::runtime_error("Flare Input: Failed to enable video input. Is another application using the card?");
		}
		
	}
        
    void getDetails(mush::core::inputConfigStruct &config) override {
        
    }
        
	void gather() {
        SetThreadName("flare10capture:");
		int64_t frames = 0;
		
		putLog("Beginning capture ...");
		if ((result = deckLinkInputAlpha->StartStreams()) != S_OK) {
			putLog("Failed to start.");
			throw std::exception();
		}
		
		inLock();
		inUnlock();

		while(true) { //deckLinkInputAlpha->Okay()) { // this should be able to stop itself yknow
			VideoFrame * videoFrame;
			IDeckLinkVideoInputFrame * videoInputFrame;
			void * ptr;
			
			{
				std::unique_lock<std::mutex> lock(listMutex);
				listCond.wait(lock, [&](){return !list.empty() || _list_disabled; });
                
                if (_list_disabled) {
                    break;
                }
                
				videoFrame = list.front();
				list.pop_front();
			}
			
			videoInputFrame = videoFrame->frame;
			auto ptr2 = inLock();
			if (ptr2 == nullptr) {
                stop();
				return;
			}
			videoInputFrame->GetBytes(&ptr);
			memcpy(ptr2.get_pointer(), ptr, (length/48)*_height*_size);

			inUnlock();
			
			videoInputFrame->Release();
			delete videoFrame;
			
			
			++frames;
		}
		
	}

	void stop() {
		putLog("Ending capture ...");
		if (deckLinkInputAlpha != NULL) {
			deckLinkInputAlpha->StopStreams();
            deckLinkInputAlpha->DisableVideoInput();
		}	

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
        taken = 0;
	}

	void setAWB() {
		flareDelegate->setAWB();
	}

	float getMiddleFrameIso() {
		return flareDelegate->getMiddleFrameIso();
	}

	float getTopFrameIso() {
		return flareDelegate->getTopFrameIso();
	}

	void isoUp(int i) {
		flareDelegate->isoUp(i);
	}

	void isoDown(int i) {
		flareDelegate->isoDown(i);
	}
	
	protected:
private:

        bool _list_disabled = false;
        
	std::shared_ptr<mush::opencl> context = nullptr;
	Flare2KCaptureDelegate * flareDelegate = nullptr;

	IDeckLinkIterator * deckLinkIterator = nullptr;
	IDeckLink * deckLinkAlpha = nullptr;
	IDeckLinkInput * deckLinkInputAlpha = nullptr;
	HRESULT result = 0;

	std::mutex listMutex;
	std::condition_variable listCond;
	std::list<VideoFrame *> list;
	unsigned int length = 0;
    
	static int taken;

    mush::bmModes bmMode = mush::bmModes::m1080p30;
	
};


#endif
