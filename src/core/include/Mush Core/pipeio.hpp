#ifndef PIPEIO_HPP
#define PIPEIO_HPP

#include <fstream>
#include <sstream>

#include "mush-core-dll.hpp"
#include <OpenEXR/ImfIO.h>

namespace Imf {
	class MUSHEXPORTS_API PipeInStream: public Imf::IStream {
	public:
		PipeInStream(std::ifstream &is, const char fileName[]);

		virtual bool read(char c[], int n);
		virtual Int64 tellg();
		virtual void seekg(Int64 pos);
		virtual void clear();
		virtual void rewind();

	private:
		std::ifstream *	_is;
		std::streamsize _pos;
	};

	class MUSHEXPORTS_API PipeOutStream: public OStream {
	public:
		PipeOutStream(std::ofstream &os, const char fileName[]);

		virtual void write(const char c[], int n);
		virtual Int64 tellp();
		virtual void seekp(Int64 pos);
		virtual void rewind();

	private:
		std::ofstream *	_os;
		std::streamsize _pos;
	};
}

#endif
