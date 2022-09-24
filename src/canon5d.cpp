#include <cstdio>

#include <iostream>

#include "canon5d.hpp"

extern "C" void putLog(std::string s);

//using namespace std;

Canon5DCaptureDelegate::Canon5DCaptureDelegate() :
	m_refCount(0),
	frames(NULL),
	framesMutex(NULL),
	framesCond(NULL) {
	}

Canon5DCaptureDelegate::~Canon5DCaptureDelegate() {
}

ULONG Canon5DCaptureDelegate::AddRef(void) {
	m_mutex.lock();
	m_refCount++;
	m_mutex.unlock();

	return (ULONG)m_refCount;
}

ULONG Canon5DCaptureDelegate::Release(void) {
	m_mutex.lock();
	m_refCount--;
	m_mutex.unlock();

	if (m_refCount == 0) {
		delete this;
		return 0;
	}

	return (ULONG)m_refCount;
}

HRESULT Canon5DCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame * videoInputFrame, IDeckLinkAudioInputPacket * audioFrame) {
	if (!videoInputFrame) {
		//fprintf(stderr, "No video frame\n");
		return S_FALSE;
	}

	if (videoInputFrame->GetFlags() & bmdFrameHasNoInputSource) {
		//fprintf(stderr, "No input signal detected\n");
		return S_FALSE;
	}

	{
		std::lock_guard<std::mutex> lock(*framesMutex);
		
		if (frames->size() > 10) {
			VideoFrame * videoFrame;
			putLog("Not keeping up!");
			videoFrame = frames->back();
			frames->pop_back();
			videoFrame->frame->Release();
			delete videoFrame;
			videoFrame = frames->back();
			frames->pop_back();
			videoFrame->frame->Release();
			delete videoFrame;
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

HRESULT Canon5DCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode * mode, BMDDetectedVideoInputFormatFlags flags) {
	//if (events == bmdVideoInputFieldDominanceChanged) {fprintf(stderr, "Field Dominance Changed! Panic!\n"); return E_FAIL;}
	return S_OK;
}

