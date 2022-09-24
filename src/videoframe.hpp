#ifndef __VIDEOFRAME_H__
#define __VIDEOFRAME_H__

#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#include "DeckLink-Win32/DeckLinkAPI.h"
#else
#include "DeckLink/DeckLinkAPI.h"
#endif

class VideoFrame {
	public:
		int id;
		IDeckLinkVideoInputFrame * frame;
};

#endif

