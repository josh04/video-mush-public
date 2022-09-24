//
//  clexr.cpp
//  clenc
//
//  Created by Jonathan Hatchett on 03/01/2012.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>

#include "hdrPFM.hpp"

bool hdrPFM::readPFMHeader(std::ifstream & is, unsigned int &width, unsigned int &height) {
		char waste;
		int scale;

		// Header (showing colour image)
		bool head = false;
		bool fail = false;
		while (!head && !fail) {
			fail = !hdrPFM::readFromPipe(is, &waste, sizeof(char));
			if (waste == 'P') {
				fail = !hdrPFM::readFromPipe(is, &waste, sizeof(char));
				if (waste == 'F') {
					head = true;
					fail = !readFromPipe(is, &waste, sizeof(char));
				}
			}
		}

		if (fail) {
			return false;
		}

		// Width and Height
		std::string ln;
		int cont = 0;
		while (cont < 2) {
			if (hdrPFM::readFromPipe(is, &waste, sizeof(char))) {
				ln.push_back(waste);
				if (waste == 0x0A) {
					++cont;
				}	
			} else {
				return false;
			}
		}
		std::stringstream s(ln);
		s >> width >> height >> scale;
		//sscanf(ln.c_str(), "%d %d %d", &width, &height, &scale);
		return true;
}

bool hdrPFM::readPFMData(std::ifstream &is, const unsigned int width, const unsigned int height, unsigned char* pixelBuffer) {
	return hdrPFM::readFromPipe(is, reinterpret_cast<char*>(pixelBuffer), sizeof(float) * width * height * 3);
}

bool hdrPFM::readPFM(std::ifstream & is, unsigned char* pixelBuffer) {
	unsigned int width, height;
	if (hdrPFM::readPFMHeader(is, width, height)) {
		return hdrPFM::readPFMData(is, width, height, pixelBuffer);
	}

	return false;
}	

bool hdrPFM::readFromPipe(std::ifstream &is, char c[], int n) {
	std::streamsize pos = 0;
	std::streamsize target = pos + n;
	while (pos < target) {
		is.read(c, target - pos);
		pos += is.gcount();

		if (is.eof()) {
			//is.clear(is.rdstate() & (ios::badbit | ios::failbit));
			return false;
		} // Throw away the eof and keep reading, the pipe may get more data later.
		if (is.fail()) {
			//is.clear(is.rdstate() & (ios::badbit | ios::eofbit));
			return false;
		} // A pipe fail doesn't appear to be such a bad thing either, keep going.
		//if (_is->fail()) {throw Iex::InputExc("Failed Pipe");}
		//if (is->bad()) {throw Iex::InputExc("Bad Pipe");} // Alright fine, so it's dead.
		if (is.bad()) {std::cout << "DEAD" << std::endl; return false; } // Alright fine, so it's dead.
	}

	return true;
}
