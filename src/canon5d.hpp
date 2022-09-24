#ifndef __CANON5D_HPP__
#define __CANON5D_HPP__

#include <list>

#include <boost/thread.hpp>
#include <mutex>
#include <condition_variable>

#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#include "DeckLink-Win32/DeckLinkAPI.h"
#else
#include "DeckLink/DeckLinkAPI.h"
#endif

#include "videoframe.hpp"

class Canon5DCaptureDelegate : public IDeckLinkInputCallback {
	public:
		Canon5DCaptureDelegate();
		~Canon5DCaptureDelegate();

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
};

#endif

