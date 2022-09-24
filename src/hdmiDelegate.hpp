#ifndef __hdmidelegate_HPP__
#define __hdmidelegate_HPP__

#include <list>
#include <mutex>
#include <condition_variable>

//#include <boost/asio.hpp>
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

class hdmiDelegate : public IDeckLinkInputCallback {
	public:
		hdmiDelegate();
		~hdmiDelegate();

		virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) { return E_NOINTERFACE; }
		virtual ULONG STDMETHODCALLTYPE AddRef(void);
		virtual ULONG STDMETHODCALLTYPE  Release(void);
		virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*, BMDDetectedVideoInputFormatFlags);
		virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);

		std::list<VideoFrame *> * frames;
		std::mutex * framesMutex;
		std::condition_variable * framesCond;
		int id;

	protected:
		ULONG m_refCount;
		std::mutex m_mutex;
	int h = 0;
};

#endif

