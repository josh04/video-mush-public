//
//  outputsplit.cpp
//  compressor
//
//  Created by AFineTapestry on 14/01/2013.
//  Copyright (c) 2013. All rights reserved.
//

#include <cstdio>

#include <iostream>
#include <exception>

#include <png.h>

using namespace std;

int writeImage(char * filename, int width, int height, unsigned char * buffer) {
	int code = 0;
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep row;
	
	// Open file for writing (binary mode)
	fp = fopen(filename, "wb");
	if (fp == NULL) {
		fprintf(stderr, "Could not open file %s for writing\n", filename);
		code = 1;
		goto finalise;
	}
	
	// Initialize write structure
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fprintf(stderr, "Could not allocate write struct\n");
		code = 1;
		goto finalise;
	}
	
	// Initialize info structure
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fprintf(stderr, "Could not allocate info struct\n");
		code = 1;
		goto finalise;
	}
	
	// Setup Exception handling
	if (setjmp(png_jmpbuf(png_ptr))) {
		fprintf(stderr, "Error during png creation\n");
		code = 1;
		goto finalise;
	}
	
	png_init_io(png_ptr, fp);
	
	// Write header (8 bit colour depth)
	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	// Set title
	/*if (title != NULL) {
	 png_text title_text;
	 title_text.compression = PNG_TEXT_COMPRESSION_NONE;
	 title_text.key = "Title";
	 title_text.text = title;
	 png_set_text(png_ptr, info_ptr, &title_text, 1);
	 }*/
	
	png_write_info(png_ptr, info_ptr);
	
	// Allocate memory for one row (3 bytes per pixel - RGB)
	row = (png_bytep)malloc(3 * width * sizeof(png_byte));
	
	// Write image data
	int x, y;
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			memcpy(row + (x * 3), buffer + (y * width * 4) + (x * 4) + 2, sizeof(unsigned char));
			memcpy(row + (x * 3) + 1, buffer + (y * width * 4) + (x * 4) + 1, sizeof(unsigned char));
			memcpy(row + (x * 3) + 2, buffer + (y * width * 4) + (x * 4), sizeof(unsigned char));
		}
		png_write_row(png_ptr, row);
	}
	
	// End write
	png_write_end(png_ptr, NULL);
	
finalise:
	if (fp != NULL) {fclose(fp);}
	if (info_ptr != NULL) {png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);}
	if (png_ptr != NULL) {png_destroy_write_struct(&png_ptr, (png_infopp)NULL);}
	if (row != NULL) {free(row);}
	
	return code;
}

const char * usage = "Usage: outputsplit width height input.pipe output%i.png\n";

int main(int argc, const char * argv[]) {
	try {
		if (argc < 4) {cerr << usage; return EXIT_FAILURE;}
		
		int width = atoi(argv[1]);
		int height = atoi(argv[2]);
		const char * inputFile = argv[3];
		const char * outputFile = argv[4];
		
		png_byte * buffer = new png_byte[width * height * 4];
		
		FILE * input = fopen(inputFile, "rb");
		
		int i = 0;
		char filename[80];
		
		while (true) {
			unsigned int read = 0;
			while (read < (sizeof(unsigned char) * width * height * 4)) {
				read += fread(buffer, sizeof(unsigned char), width * height * 4, input);
				if (ferror(input) || feof(input)) {goto exit;} // Yey, legitimate goto
			}
			if (snprintf(filename, 80, outputFile, i) < 0) {cerr << "snprintf error" << endl; return EXIT_FAILURE;}
			if (writeImage(filename, width, height, buffer)) {cerr << "writeImage error" << endl; return EXIT_FAILURE;}
			cout << ".";
			
			++i;
		}
		
exit:	delete[] buffer;
		
		fclose(input);
		
		return EXIT_SUCCESS;
	} catch (exception & e) {
		cerr << e.what() << endl;
		return EXIT_FAILURE;
	} catch (...) {
		cerr << "Unknown exception" << endl;
		return EXIT_FAILURE;
	}
}
