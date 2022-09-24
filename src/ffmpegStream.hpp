#ifndef FFMPEGSTREAM_HPP
#define FFMPEGSTREAM_HPP

#include <memory>
#include <sstream>
#include <boost/filesystem.hpp>

#include "ffmpegWrapper.hpp"

#include "ConfigStruct.hpp"
class ldrRingBuffer;
class metadataRingBuffer;

class ffmpegStream : public ffmpegWrapper {
public:
	ffmpegStream() {}

	~ffmpegStream() {}	

	void init(std::shared_ptr<ldrRingBuffer> output, std::shared_ptr<metadataRingBuffer> metadata, const hdr::config::outputConfigStruct &outputConfig);

	void go();
	void go2();
    void go3();
	
	std::string checkExists(std::string final, std::string original = "", unsigned int count = 1) {
		if (original == "") {
			original = final;
		}
		boost::filesystem::path pathT(final);
		if (boost::filesystem::exists(pathT)) {
			boost::filesystem::path pathTo(original);
			boost::filesystem::path extn = pathTo.extension();
			pathTo.replace_extension();
			std::stringstream strm;
			strm << " " << count;
			pathTo += strm.str();
			pathTo.replace_extension(extn);
			return checkExists(pathTo.string(), original, ++count);
		} else {
			return final;
		}
	}

private:
	std::shared_ptr<ldrRingBuffer> output = nullptr;
	std::shared_ptr<metadataRingBuffer> metadata = nullptr;
	unsigned int width = 0, height = 0;
	unsigned int bitrate = 0;
	float fps = 0;

	std::string filename;
	std::string path;
};

#endif