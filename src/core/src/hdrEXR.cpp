//
//  clexr.cpp
//  clenc
//
//  Created by Jonathan Hatchett on 03/01/2012.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#define __CL_ENABLE_EXCEPTIONS


//#include "pfm.hpp"
#include <OpenEXR/ImfRgbaFile.h>
#include <vector>
#include "opencl.hpp"
#include "hdrEXR.hpp"
#include "pipeio.hpp"

#include <OpenEXR/ImfCompression.h>
#include <OpenEXR/ImfLineOrder.h>
#include <OpenEXR/ImathVec.h>
#include <OpenEXR/ImfVecAttribute.h>
#include <OpenEXR/ImfFloatAttribute.h>
#include <OpenEXR/ImfHeader.h>

extern "C" void putLog(std::string s);

extern "C" void putLog(std::string s);

void hdrEXR::ReadEXR(mush::buffer& buf, const char * filename) {
	Imf::RgbaInputFile in(filename);
	
	Imath::Box2i win = in.dataWindow();
	
	Imath::V2i dim(win.max.x - win.min.x + 1, win.max.y - win.min.y + 1);
		
	int dx = win.min.x;
	int dy = win.min.y;
	auto& header = in.header();

	//if (!header.readsNothing()) {
		auto loc = in.header().findTypedAttribute<Imf::V3fAttribute>("mush_camera_position");
		auto tpf = in.header().findTypedAttribute<Imf::V3fAttribute>("mush_camera_theta_phi_fov");
		if (loc && tpf) {
			cl_float3 theta_phi_fov = { tpf->value().x, tpf->value().y, tpf->value().z };

			buf.set_camera_position({ loc->value().x, loc->value().y, loc->value().z }, theta_phi_fov);
		} else {
			buf.set_no_camera_position();
		}
	//} else {
	//	buf.set_no_camera_position();
	//}
    
    Imf::Rgba * pixelBuffer = (Imf::Rgba *)buf.get_pointer();
	in.setFrameBuffer(pixelBuffer - dx - dy * dim.x, 1, dim.x);
	in.readPixels(win.min.y, win.max.y);
	//update width and height based on info	
//	if (width) {*width = dim.x;}
//	if (height) {*height = dim.y;}
}

void hdrEXR::ReadEXR(unsigned char * pixelBuffer, Imf::IStream & is) {
	Imf::RgbaInputFile in(is);
	
	Imath::Box2i win = in.dataWindow();
	
	Imath::V2i dim(win.max.x - win.min.x + 1, win.max.y - win.min.y + 1);
	
	int dx = win.min.x;
	int dy = win.min.y;
	
	in.setFrameBuffer(reinterpret_cast<Imf::Rgba *>(pixelBuffer) - dx - dy * dim.x, 1, dim.x);
	in.readPixels(win.min.y, win.max.y);
}

void hdrEXR::ReadSize(const char * filename, unsigned int &width, unsigned int &height) {
	Imf::RgbaInputFile in(filename);
	
	Imath::Box2i win = in.dataWindow();
	
//	Imath::V2i dim(win.max.x - win.min.x + 1, win.max.y - win.min.y + 1);
	width = win.max.x - win.min.x + 1;
	height = win.max.y - win.min.y + 1;


}

void hdrEXR::CloseEXR(Imf::Rgba ** pixelBuffer) {
	if ((*pixelBuffer) == NULL) {throw std::runtime_error("Buffer NULL. Not created or deleted.");}
	
	delete [] *pixelBuffer;
	
	*pixelBuffer = NULL;
}

void hdrEXR::CreateEXR(Imf::Rgba ** pixelBuffer, unsigned int width, unsigned int height) {
	if ((*pixelBuffer) != NULL) {throw std::runtime_error("Buffer not NULL. Already created or deleted.");}
	
	(*pixelBuffer) = new Imf::Rgba[width * height];
}

void hdrEXR::WriteEXR(Imf::Rgba * pixelBuffer, const char *filename, unsigned int width, unsigned int height) {
	if (pixelBuffer == NULL) {throw std::runtime_error("Buffer NULL. Not created or deleted.");}
	
	Imf::RgbaOutputFile file(filename, width, height, Imf::WRITE_RGBA, 1.0f, Imath::V2f (0, 0), 1.0f, Imf::INCREASING_Y, Imf::RLE_COMPRESSION);
	file.setFrameBuffer(pixelBuffer, 1, width);
	//file.header.
	file.writePixels(height);
}

void hdrEXR::WriteEXR(Imf::Rgba * pixelBuffer, const char *filename, unsigned int width, unsigned int height, cl_float3 position, cl_float3 theta_phi_fov) {
	if (pixelBuffer == NULL) { 
		throw std::runtime_error("Buffer was null at EXR write.");
	}

	Imf::Header hd(width, height, 1.0f, Imath::V2f(0, 0), 1.0f, Imf::INCREASING_Y, Imf::RLE_COMPRESSION);

	hd.insert("mush_camera_position", Imf::V3fAttribute(Imath::V3f(position.s[0], position.s[1], position.s[2])));
	hd.insert("mush_camera_theta_phi_fov", Imf::V3fAttribute(Imath::V3f(theta_phi_fov.s[0], theta_phi_fov.s[1], theta_phi_fov.s[2])));

	Imf::RgbaOutputFile file(filename, hd, Imf::WRITE_RGBA);
	file.setFrameBuffer(pixelBuffer, 1, width);

	file.writePixels(height);
}

void hdrEXR::WriteEXR(boost::shared_array<Imf::Rgba> pixelBuffer, Imf::OStream & is, unsigned int width, unsigned int height) {
//	Imf::Header header(width, height, 1, Imath::V2f(0, 0), 1, Imf::INCREASING_Y, Imf::NO_COMPRESSION);
	Imf::Header header(width, height);
	Imf::RgbaOutputFile file(is, header);
	file.setFrameBuffer(pixelBuffer.get(), 1, width);
	file.writePixels(height);
}

void hdrEXR::WriteEXR(boost::shared_array<half> pixelBuffer, Imf::OStream & is, unsigned int width, unsigned int height) {
//	Imf::Header header(width, height, 1, Imath::V2f(0, 0), 1, Imf::INCREASING_Y, Imf::NO_COMPRESSION);
	Imf::Header header(width, height);
	Imf::RgbaOutputFile file(is, header);
	file.setFrameBuffer(reinterpret_cast<Imf::Rgba*>(pixelBuffer.get()), 1, width);
	file.writePixels(height);
}

hdrEXR::clToEXR::clToEXR() {

}

void hdrEXR::clToEXR::init(shared_ptr<mush::opencl> context) {
    this->context = context;
	queue = context->getQueue();
	kernel = context->getKernel("floatToHalf");
}

void hdrEXR::clToEXR::write(mush::buffer buffer, std::string filename, unsigned int width, unsigned int height) {
	kernel->setArg(0, buffer.get_image());

    
    auto halfs = context->buffer(sizeof(cl_half) * 4 * width * height, CL_MEM_WRITE_ONLY, false);
    kernel->setArg(1, *halfs);
    
	try {
		cl::Event event;
		queue->enqueueNDRangeKernel(*kernel, cl::NullRange, cl::NDRange(width, height), cl::NullRange, NULL, &event);
		event.wait();
	} catch (cl::Error& e) {
		std::stringstream strm;
		strm << "CL Error in " << e.what() << ". Error Code: " << e.err();
		putLog(strm.str());
		return;
	}

	Imf::Rgba * pixelBuffer = (Imf::Rgba *)malloc(sizeof(Imf::Rgba) * width * height);
	queue->enqueueReadBuffer(*halfs, CL_TRUE, 0, sizeof(cl_half) * 4 * width * height, pixelBuffer);
    delete halfs;

	if (buffer.has_camera_position()) {
		hdrEXR::WriteEXR(pixelBuffer, filename.c_str(), width, height, buffer.get_camera_position(), buffer.get_theta_phi_fov());
	} else {
		hdrEXR::WriteEXR(pixelBuffer, filename.c_str(), width, height);
	}
	free(pixelBuffer);
}
