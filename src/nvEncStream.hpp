#ifndef NVENCSTREAM_HPP
#define NVENCSTREAM_HPP

#include <memory>

#include <cuda.h>

#include "ConfigStruct.hpp"
#include "nvEncHelper.hpp"

#include "nvEncodeAPI.h"

class ldrRingBuffer;
class metadataRingBuffer;

class nvEncStream {
public:
	nvEncStream() {

	}

	~nvEncStream() {

	}

	void init(std::shared_ptr<ldrRingBuffer> output, std::shared_ptr<metadataRingBuffer> metadata, const mush::core::outputConfigStruct &outputConfig);

	inline bool compareGUIDs(GUID guid1, GUID guid2)
	{
		if (guid1.Data1 == guid2.Data1 &&
			guid1.Data2 == guid2.Data2 &&
			guid1.Data3 == guid2.Data3 &&
			guid1.Data4[0] == guid2.Data4[0] &&
			guid1.Data4[1] == guid2.Data4[1] &&
			guid1.Data4[2] == guid2.Data4[2] &&
			guid1.Data4[3] == guid2.Data4[3] &&
			guid1.Data4[4] == guid2.Data4[4] &&
			guid1.Data4[5] == guid2.Data4[5] &&
			guid1.Data4[6] == guid2.Data4[6] &&
			guid1.Data4[7] == guid2.Data4[7])
		{
			return true;
		}

		return false;
	}

	void go();
	void go2();

	private:
		std::shared_ptr<ldrRingBuffer> output = nullptr;
		std::shared_ptr<metadataRingBuffer> metadata = nullptr;
		unsigned int width = 0, height = 0;
		unsigned int bitrate = 0;
		float fps = 0;

		std::string filename;
		std::string path;

		CUcontext m_cuContext;
};

#endif