//
//  clexr.h
//  clenc
//
//  Created by Jonathan Hatchett on 03/01/2012.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef EXR_HPP
#define EXR_HPP

#define __CL_ENABLE_EXCEPTIONS

#include <string>
#include <vector>
#include <OpenEXR/ImfRgbaFile.h>
#include <boost/shared_array.hpp>

#include "opencl.hpp"
#include "mushBuffer.hpp"

#include <vector>

#include <memory>
using std::shared_ptr;
using std::make_shared;

#include "mush-core-dll.hpp"


namespace hdrEXR {
	
	class MUSHEXPORTS_API clToEXR {
        std::shared_ptr<mush::opencl> context;
		cl::CommandQueue * queue;
		cl::Kernel * kernel;
		public:
			clToEXR();
			clToEXR(const clToEXR &cl);
			~clToEXR(){}

			void init(shared_ptr<mush::opencl> context);

            void write(mush::buffer, std::string, unsigned int width, unsigned int height);
 	};
	
    MUSHEXPORTS_API void ReadEXR(mush::buffer& buf, const char * filename);
	MUSHEXPORTS_API void ReadEXR(unsigned char * pixelBuffer, Imf::IStream & is);
	MUSHEXPORTS_API void ReadSize(const char * filename, unsigned int &width, unsigned int &height);

	MUSHEXPORTS_API void CreateEXR(Imf::Rgba ** pixelBuffer, unsigned int width, unsigned int height);

	MUSHEXPORTS_API void CloseEXR(Imf::Rgba ** pixelBuffer);

	MUSHEXPORTS_API void WriteEXR(Imf::Rgba * pixelBuffer, const char * filename, unsigned int width, unsigned int height);

	MUSHEXPORTS_API void WriteEXR(Imf::Rgba * pixelBuffer, const char * filename, unsigned int width, unsigned int height, cl_float3 position, cl_float3 theta_phi_fov);

	MUSHEXPORTS_API void WriteEXR(boost::shared_array<Imf::Rgba> pixelBuffer, Imf::OStream & is, unsigned int width, unsigned int height);
	MUSHEXPORTS_API void WriteEXR(boost::shared_array<half> pixelBuffer, Imf::OStream & is, unsigned int width, unsigned int height);

}

#endif
