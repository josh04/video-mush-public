
//
//  clexr.h
//  clenc
//
//  Created by Jonathan Hatchett on 03/01/2012.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef PFM_HPP
#define PFM_HPP

#include <fstream>

namespace hdrPFM {
	bool readPFMHeader(std::ifstream & is, unsigned int &width, unsigned int &height);
	bool readPFMData(std::ifstream & is, const unsigned int width, const unsigned int height, unsigned char* pixelBuffer);
	bool readPFM(std::ifstream & is, unsigned char* pixelBuffer);
	bool readFromPipe(std::ifstream &is, char c[], int n);
}

#endif
