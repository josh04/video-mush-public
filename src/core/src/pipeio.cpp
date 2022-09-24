#include <iostream>

#include <OpenEXR/Iex.h>

#include "pipeio.hpp"

namespace {
    using namespace std;
}

namespace Imf {
	PipeInStream::PipeInStream(std::ifstream &is, const char fileName[]):
    IStream(fileName),
    _is(&is),
	_pos(0) {}
	
	bool PipeInStream::read(char c[], int n) {
		streamsize target = _pos + n;
		
		while (_pos < target) {
			_is->read(c, target - _pos);
			_pos += _is->gcount();

			if (_is->eof()) {_is->clear(_is->rdstate() & (ios::badbit | ios::failbit));} // Throw away the eof and keep reading, the pipe may get more data later.
			if (_is->fail()) {_is->clear(_is->rdstate() & (ios::badbit | ios::eofbit));} // A pipe fail doesn't appear to be such a bad thing either, keep going.
			//if (_is->fail()) {throw Iex::InputExc("Failed Pipe");}
			if (_is->bad()) {throw Iex::InputExc("Bad Pipe");} // Alright fine, so it's dead.
		}

		return true;
	}
	
	Int64 PipeInStream::tellg() {
		return _pos;
	}
	
	void PipeInStream::seekg(Int64 pos) {
		streamsize target = pos;
		
		if (target < _pos) {throw Iex::InputExc("Backwards Seek");}
		
		while (_pos < target) {
			_is->get();
			_pos++;
			
//			if (_is->eof()) {_is->clear(_is->rdstate() & (ios::badbit | ios::failbit));}
//			if (_is->fail()) {_is->clear(_is->rdstate() & (ios::badbit | ios::eofbit));}
			if (_is->eof()) {throw Iex::InputExc("Closed Pipe");}
			if (_is->fail()) {throw Iex::InputExc("Failed Pipe");}
			if (_is->bad()) {throw Iex::InputExc("Bad Pipe");}
		}
	}
	
	void PipeInStream::clear() {
		_is->clear();
	}
	
	void PipeInStream::rewind() {
		_pos = 0;
	}
	
	PipeOutStream::PipeOutStream(ofstream &os, const char fileName[]):
    OStream(fileName),
    _os(&os),
	_pos(0) {}
	
	void PipeOutStream::write(const char c[], int n) {
		_os->write(c, n);
		
		if (_os->fail()) {throw Iex::InputExc("Failed Pipe");}
		if (_os->bad()) {throw Iex::InputExc("Bad Pipe");}
		
		_pos += n;
	}
	
	
	Int64 PipeOutStream::tellp() {
		return _pos;
	}
	
	
	void PipeOutStream::seekp(Int64 pos) {
		streamsize target = pos;
		
		if (target < _pos) {throw Iex::InputExc("Backwards Seek");}
		
		while (_pos < target) {
			_os->put(0);
			
			if (_os->fail()) {throw Iex::InputExc("Failed Pipe");}
			if (_os->bad()) {throw Iex::InputExc("Bad Pipe");}
			
			_pos++;
		}
	}
	
	void PipeOutStream::rewind() {
		_pos = 0;
	}
}
