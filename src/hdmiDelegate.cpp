#include <cstdio>
#include <iostream>

#include "hdmiDelegate.hpp"

extern "C" void putLog(std::string s);

hdmiDelegate::hdmiDelegate() :
	m_refCount(0),
	frames(NULL),
	framesMutex(NULL),
	framesCond(NULL)
		 {
	}

hdmiDelegate::~hdmiDelegate() {
}

ULONG hdmiDelegate::AddRef(void) {
	m_mutex.lock();
	m_refCount++;
	m_mutex.unlock();

	return (ULONG)m_refCount;
}

ULONG hdmiDelegate::Release(void) {
	m_mutex.lock();
	m_refCount--;
	m_mutex.unlock();

	if (m_refCount == 0) {
		delete this;
		return 0;
	}

	return (ULONG)m_refCount;
}

HRESULT hdmiDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame * videoInputFrame, IDeckLinkAudioInputPacket * audioFrame) {
	
	if (!videoInputFrame) {
		putLog("HDMI: No video frame");
		return S_FALSE;
	}

	if (videoInputFrame->GetFlags() & bmdFrameHasNoInputSource) {
		putLog("HDMI: No input signal detected");
		return S_FALSE;
	}
	
	{
		std::lock_guard<std::mutex> lock(*framesMutex);
		
		if (frames->size() > 12) {
			VideoFrame * videoFrame;
			putLog("Not keeping up!");
				videoFrame = frames->back();
				frames->pop_back();
				videoFrame->frame->Release();
				delete videoFrame;
		} else {
			videoInputFrame->AddRef();
			
			VideoFrame * videoFrame = new VideoFrame();
			videoFrame->id = id;
			videoFrame->frame = videoInputFrame;
			
			frames->push_back(videoFrame);
		}
	}

	framesCond->notify_all();

	return S_OK;
}

HRESULT hdmiDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode * mode, BMDDetectedVideoInputFormatFlags flags) {
	//if (events == bmdVideoInputFieldDominanceChanged) {fprintf(stderr, "Field Dominance Changed! Panic!\n"); return E_FAIL;}
	return S_OK;
}
