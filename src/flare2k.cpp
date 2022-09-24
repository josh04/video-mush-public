#include <cstdio>
#include <iostream>
#include <memory>

#include <Mush Core/mushLog.hpp>
#include <boost/asio.hpp>
#include "flare2k.hpp"
#include "flare10capture.hpp"

int hdrFlare10::taken = 0;

Flare2KCaptureDelegate::Flare2KCaptureDelegate(unsigned int height, int exposures, const char* flareCOMPort) :
m_refCount(0),
frames(NULL),
framesMutex(NULL),
framesCond(NULL),
height(height),
exposures(exposures),
lower(60),
middle(600),
upper(2000),
port(nullptr)
{
	//putLog("Woah");
	//Sleep(10);
    try {
        port = std::unique_ptr<boost::asio::serial_port>(new boost::asio::serial_port(io, flareCOMPort));
    
		if (port != nullptr) {
			if (port->is_open()) {
				port->set_option(boost::asio::serial_port_base::baud_rate(115200));
				port->set_option(boost::asio::serial_port_base::character_size(8));
				port->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
				port->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
				port->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));


				write(*port, boost::asio::buffer("exd 2\r\n", 7));
				write(*port, boost::asio::buffer("awb\r\n", 5));
				const char * command = getExposureCommand(lower);
				write(*port, boost::asio::buffer(command, 8));

				if (height == 720) {
					//write(*port, boost::asio::buffer("vid 07\r\n", 8));
				} else {
					write(*port, boost::asio::buffer("vid 0c\r\n", 8));
				}
			}
		}
	} catch (std::runtime_error& e) {
		putLog("Warning: USB serial adapter not connected");
		port = nullptr;
	} catch (std::exception& e) {
		putLog("Warning: unhandled exception");
	}
}

Flare2KCaptureDelegate::Flare2KCaptureDelegate(unsigned int height, int exposures) :
m_refCount(0),
frames(NULL),
framesMutex(NULL),
framesCond(NULL),
height(height),
exposures(exposures),
lower(60),
middle(600),
upper(2000),
port(nullptr)
{
}

Flare2KCaptureDelegate::~Flare2KCaptureDelegate() {
}

ULONG Flare2KCaptureDelegate::AddRef(void) {
	m_mutex.lock();
	m_refCount++;
	m_mutex.unlock();

	return (ULONG)m_refCount;
}

ULONG Flare2KCaptureDelegate::Release(void) {
	m_mutex.lock();
	m_refCount--;
	m_mutex.unlock();

	if (m_refCount == 0) {
		delete this;
		return 0;
	}

	return (ULONG)m_refCount;
}

HRESULT Flare2KCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame * videoInputFrame, IDeckLinkAudioInputPacket * audioFrame) {
	
//	if (!setup) {
//		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
//		setup = true;
//	}
	
	if (!videoInputFrame) {
		putLog("Flare2k: No video frame");
		return S_FALSE;
	}

	static bool log = false;
	if (videoInputFrame->GetFlags() & bmdFrameHasNoInputSource) {
		if (!log) {
			putLog("Flare2k: No input signal detected");
			log = true;
		}
		return S_FALSE;
	}
	log = false;
	
	if (port != nullptr) {
		if (exposures == 2) {
			if (h % 2 == 0) {
				const char * command = getExposureCommand(lower);
				write(*port, boost::asio::buffer(command, 8), boost::asio::transfer_all());
			} else {
				const char * command = getExposureCommand(middle);
				write(*port, boost::asio::buffer(command, 8), boost::asio::transfer_all());
			}
			h = (h + 1) % 2;
		} else if (exposures == 3) {
			if (h % 3 == 0) {
				const char * command = getExposureCommand(middle);
				//write(*port, boost::asio::buffer((std::string(command) + std::string("")).c_str(), 7), boost::asio::transfer_all());
				//write(*port, boost::asio::buffer("agn 0\r", 6), boost::asio::transfer_all());
				write(*port, boost::asio::buffer(command, 7), boost::asio::transfer_all());
			} else if (h % 3 == 1){
				const char * command = getExposureCommand(upper);
				//write(*port, boost::asio::buffer((std::string(command) + std::string("")).c_str(), 7), boost::asio::transfer_all());
				//write(*port, boost::asio::buffer("agn 0\r", 6), boost::asio::transfer_all());
				write(*port, boost::asio::buffer(command, 7), boost::asio::transfer_all());
			} else {
				const char * command = getExposureCommand(lower);
				//write(*port, boost::asio::buffer((std::string(command) + std::string("")).c_str(), 7), boost::asio::transfer_all());
				//write(*port, boost::asio::buffer("agn 3\r", 6), boost::asio::transfer_all());
				write(*port, boost::asio::buffer(command, 7), boost::asio::transfer_all());
			}
			h = (h + 1) % 3;
		}
	}

	{
		std::lock_guard<std::mutex> lock(*framesMutex);
		
		if (frames->size() > 12) {
			VideoFrame * videoFrame;
			//putLog("Not keeping up!");

			if (frames_dropped == 13) {
				putLog("Warning: Dropping frames.");
			}

			if (exposures == 2) {
				videoFrame = frames->back();
				frames->pop_back();
				videoFrame->frame->Release();
				delete videoFrame;
				frames_dropped += 2;
			}
			if (exposures == 3) {
				videoFrame = frames->back();
				frames->pop_back();
				videoFrame->frame->Release();
				delete videoFrame;
				videoFrame = frames->back();
				frames->pop_back();
				videoFrame->frame->Release();
				delete videoFrame;
				frames_dropped += 3;
			}
			
		} else {
			if (frames_dropped > 0) {
				char text[64];
				snprintf(text, 64, "Dropped %d frames.", frames_dropped);
				putLog(text);
				frames_dropped = 0;
			}

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

const char * Flare2KCaptureDelegate::getExposureCommand(int exposureTime) {
	switch (exposureTime) {
	case 2000:
		return "exf 00\r\n";
		break;
	case 1600:
		return "exf 01\r\n";
		break;
	case 1400:
		return "exf 02\r\n";
		break;
	case 1200:
		return "exf 03\r\n";
		break;
	case 1000:
		return "exf 04\r\n";
		break;
	case 800:
		return "exf 05\r\n";
		break;
	case 700:
		return "exf 06\r\n";
		break;
	case 600:
		return "exf 07\r\n";
		break;
	case 500:
		return "exf 08\r\n";
		break;
	case 400:
		return "exf 09\r\n";
		break;
	case 350:
		return "exf 0a\r\n";
		break;
	case 300:
		return "exf 0b\r\n";
		break;
	case 250:
		return "exf 0c\r\n";
		break;
	case 210:
		return "exf 0d\r\n";
		break;
	case 180:
		return "exf 0e\r\n";
		break;
	case 150:
		return "exf 0f\r\n";
		break;
	case 120:
		return "exf 10\r\n";
		break;
	case 100:
		return "exf 11\r\n";
		break;
	case 90:
		return "exf 12\r\n";
		break;
	case 75:
		return "exf 13\r\n";
		break;
	case 60:
		return "exf 14\r\n";
		break;
	case 50:
		return "exf 15\r\n";
		break;
	case 40:
		return "exf 16\r\n";
		break;
	case 33:
		return "exf 17\r\n";
		break;
	case 29:
		return "exf 18\r\n";
		break;
	case 25:
		return "exf 19\r\n";
		break;
	}
	return "";
}

float Flare2KCaptureDelegate::getMiddleFrameIso() {
	return (float) middle / lower;
}

float Flare2KCaptureDelegate::getTopFrameIso() {
	return (float) upper / lower;
}

void Flare2KCaptureDelegate::isoUp(int i) {
	unsigned int exposureTime = 0;
	switch (i) {
	case 0:
		exposureTime = lower;
		break;
	case 1:
		exposureTime = middle;
		break;
	case 2: 
		exposureTime = upper;
		break;
	}

	switch (exposureTime) {
	case 2000:
		exposureTime = 2000;
		break;
	case 1600:
		exposureTime = 2000;
		break;
	case 1400:
		exposureTime = 1600;
		break;
	case 1200:
		exposureTime = 1400;
		break;
	case 1000:
		exposureTime = 1200;
		break;
	case 800:
		exposureTime = 1000;
		break;
	case 700:
		exposureTime = 800;
		break;
	case 600:
		exposureTime = 700;
		break;
	case 500:
		exposureTime = 600;
		break;
	case 400:
		exposureTime = 500;
		break;
	case 350:
		exposureTime = 400;
		break;
	case 300:
		exposureTime = 350;
		break;
	case 250:
		exposureTime = 300;
		break;
	case 210:
		exposureTime = 250;
		break;
	case 180:
		exposureTime = 210;
		break;
	case 150:
		exposureTime = 180;
		break;
	case 120:
		exposureTime = 150;
		break;
	case 100:
		exposureTime = 120;
		break;
	case 90:
		exposureTime = 100;
		break;
	case 75:
		exposureTime = 90;
		break;
	case 60:
		exposureTime = 75;
		break;
	case 50:
		exposureTime = 60;
		break;
	case 40:
		exposureTime = 50;
		break;
	case 33:
		exposureTime = 40;
		break;
	case 29:
		exposureTime = 33;
		break;
	case 25:
		exposureTime = 29;
		break;
	}

	std::stringstream strm;
	switch (i) {
	case 0:
		lower = exposureTime;
		strm << "lower: " << lower;
		break;
	case 1:
		middle = exposureTime;
		strm << "middle: " << middle;
		break;
	case 2:
		upper = exposureTime;
		strm << "upper: " << upper;
		break;
	}
	putLog(strm.str().c_str());
}

void Flare2KCaptureDelegate::isoDown(int i) {
	unsigned int exposureTime = 0;
	switch (i) {
	case 0:
		exposureTime = lower;
		break;
	case 1:
		exposureTime = middle;
		break;
	case 2:
		exposureTime = upper;
		break;
	}

	switch (exposureTime) {
	case 2000:
		exposureTime = 1600;
		break;
	case 1600:
		exposureTime = 1400;
		break;
	case 1400:
		exposureTime = 1200;
		break;
	case 1200:
		exposureTime = 1000;
		break;
	case 1000:
		exposureTime = 800;
		break;
	case 800:
		exposureTime = 700;
		break;
	case 700:
		exposureTime = 600;
		break;
	case 600:
		exposureTime = 500;
		break;
	case 500:
		exposureTime = 400;
		break;
	case 400:
		exposureTime = 350;
		break;
	case 350:
		exposureTime = 300;
		break;
	case 300:
		exposureTime = 250;
		break;
	case 250:
		exposureTime = 210;
		break;
	case 210:
		exposureTime = 180;
		break;
	case 180:
		exposureTime = 150;
		break;
	case 150:
		exposureTime = 120;
		break;
	case 120:
		exposureTime = 100;
		break;
	case 100:
		exposureTime = 90;
		break;
	case 90:
		exposureTime = 75;
		break;
	case 75:
		exposureTime = 60;
		break;
	case 60:
		exposureTime = 50;
		break;
	case 50:
		exposureTime = 40;
		break;
	case 40:
		exposureTime = 33;
		break;
	case 33:
		exposureTime = 29;
		break;
	case 29:
		exposureTime = 25;
		break;
	case 25:
		exposureTime = 25;
		break;
	}

	std::stringstream strm;
	switch (i) {
	case 0:
		lower = exposureTime;
		strm << "lower: " << lower;
		break;
	case 1:
		middle = exposureTime;
		strm << "middle: " << middle;
		break;
	case 2:
		upper = exposureTime;
		strm << "upper: " << upper;
		break;
	}
	putLog(strm.str().c_str());
}

HRESULT Flare2KCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode * mode, BMDDetectedVideoInputFormatFlags flags) {
	//if (events == bmdVideoInputFieldDominanceChanged) {fprintf(stderr, "Field Dominance Changed! Panic!\n"); return E_FAIL;}
	return S_OK;
}

void Flare2KCaptureDelegate::setAWB() {
	if (port != nullptr) {
		write(*port, boost::asio::buffer("awb\r\n", 5));
	}
}

