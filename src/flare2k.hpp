#ifndef __FLARE2K_HPP__
#define __FLARE2K_HPP__

#include <list>
#include <mutex>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#include "DeckLink-Win32/DeckLinkAPI.h"
#else
#include "DeckLink/DeckLinkAPI.h"
#endif


#include "videoframe.hpp"

class Flare2KCaptureDelegate : public IDeckLinkInputCallback {
	public:
		Flare2KCaptureDelegate(unsigned int height, int exposures, const char* flareCOMPort);
		Flare2KCaptureDelegate(unsigned int height, int exposures);
		~Flare2KCaptureDelegate();

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
		virtual ULONG STDMETHODCALLTYPE AddRef(void);
		virtual ULONG STDMETHODCALLTYPE  Release(void);
		virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
		virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);

		void setAWB();
		void isoUp(int i);
		void isoDown(int i);
		float getMiddleFrameIso();
		float getTopFrameIso();

		const char * getExposureCommand(int exposureTime);
		std::list<VideoFrame *> * frames;
		std::mutex * framesMutex;
		std::condition_variable * framesCond;
		int id;

	protected:
		ULONG m_refCount;
		std::mutex m_mutex;
		boost::asio::io_service io;
		std::unique_ptr<boost::asio::serial_port> port;
	int h = 0;
	
	unsigned int height;
	int exposures;
	unsigned int lower, middle, upper;
	int64_t time;
	bool setup = false;
//	const char exf0 [8] = { 'e', 'x', 'f' , ' ', '0', '0', '\r', '\n' };
//	const char exf4 [8] = { 'e', 'x', 'f' , ' ', '0', '8', '\r', '\n' };
//	const char exf16[8] = {'e', 'x', 'f', ' ', '1', '5', '\r', '\n' };
	int frames_dropped = 0;
};

#endif

