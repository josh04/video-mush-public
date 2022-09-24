/*
 * This file is part of exr2pfm - a command-line utility for converting
 * OpenEXR image files to and from the Portable FloatMap (PFM) format.
 *
 * https://projects.forum.nokia.com/exr2pfm
 *
 * Copyright (C) 2011 Tomi Aarnio (tomi.p.aarnio 'at' gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this source code; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * This software is based on the OpenEXR image file format and library
 * by Industrial Light & Magic, see http://www.openexr.org.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ImfRgbaFile.h>              // EXR loading
#include <ImfArray.h>
#include <ImfOutputFile.h>            // EXR saving
#include <ImfChannelList.h>
using namespace Imf;
using namespace std;             
using namespace Imath;
////////////////////////////////////////////////////////////////////////////////
// Loads an EXR file into an Array<Rgba>.
////////////////////////////////////////////////////////////////////////////////
static Array<Rgba>* loadEXRFile (const char* filename, int& width, int& height, bool& flipY) 
{
  RgbaInputFile exrFile (filename);
  Box2i dw = exrFile.dataWindow();
  width = dw.max.x - dw.min.x + 1;
  height = dw.max.y - dw.min.y + 1;
  Array<Rgba>* imageData = new Array<Rgba>(height*width);
  imageData->resizeErase(height*width);
  exrFile.setFrameBuffer(imageData[0] - dw.min.x - dw.min.y * width, 1, width);
  exrFile.readPixels(dw.min.y, dw.max.y);
  flipY = exrFile.lineOrder()==0? true : false;
  return imageData;
}
////////////////////////////////////////////////////////////////////////////////
// Saves an EXR file from Array<Rgba> data.
////////////////////////////////////////////////////////////////////////////////
static int saveEXRFile (const char *filename, 
                        const int width, 
                        const int height, 
                        Array<Rgba>* imageData) 
{
  if (filename == 0 || imageData == 0) {
    return 0;
  }
  // prepare header
  Header header (width, height);
  header.channels().insert ("R", Channel (HALF));
  header.channels().insert ("G", Channel (HALF));
  header.channels().insert ("B", Channel (HALF));
  // create file
  OutputFile exrFile (filename, header);
  // insert frame buffer
  FrameBuffer frameBuffer;
  frameBuffer.insert ("R",                                                                      // name
    Slice (HALF,                                                                                                                // type
    (char *) &(((Rgba*)imageData[0])->r),               // base
    sizeof (Rgba),                                                                                                      // xStride
    sizeof (Rgba) * width));                                                            // yStride
  frameBuffer.insert ("G",                                                                      // name
    Slice (HALF,                                                                                                                // type
    (char *) &(((Rgba*)imageData[0])->g),               // base
    sizeof (Rgba),                                                                                                      // xStride
    sizeof (Rgba) * width));                                                            // yStride
  frameBuffer.insert ("B",                                                                      // name
    Slice (HALF,                                                                                                                // type
    (char *) &(((Rgba*)imageData[0])->b),               // base
    sizeof (Rgba),                                                                                          // xStride
    sizeof (Rgba) * width));                                                            // yStride
  exrFile.setFrameBuffer (frameBuffer);
  exrFile.writePixels (height);
  return 1;
}
////////////////////////////////////////////////////////////////////////////////
// Saves a Portable FloatMap (PFM) file from raw float RGB or grayscale data.
////////////////////////////////////////////////////////////////////////////////
static int savePFMFile(const char *filename,
                       const int width, 
                       const int height,
                       const bool flipY,
                       Array<Rgba>* imageData)
{
  if (filename == 0 || imageData == 0) {
    return 0;
  }
  printf("Writing %s...\n", filename);
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) {
    printf("Unable to create file %s.\n", filename);
    return -1;
  }
  fprintf(fp, "PF\n");
  printf("...saving as a color PFM file...\n");
  fprintf(fp, "%d %d\n", width, height);
  fprintf(fp, "-1\n");
  printf("...width %d, height %d, channels %d...\n", width, height, 3);
  int yStart = 0;
  int yStep = 1;
  int yStop = height;
  if (flipY) {
    printf("...flipping scanline order...\n");
    yStart = height-1;
    yStep = -1;
    yStop = 0;
  }
  float *imgdata = (float *)malloc(sizeof(float)*width*height*3);
  Rgba *rgbaData = imageData[0];
  for (int y=yStart, i=0; y != yStop; y += yStep) {
    for (int x=0; x < width; x++, i++) {
      float r = rgbaData[y*width + x].r;
      float g = rgbaData[y*width + x].g;
      float b = rgbaData[y*width + x].b;
      imgdata[i*3    ] = r;
      imgdata[i*3 + 1] = g;
      imgdata[i*3 + 2] = b;
    }
  }
  fwrite(imgdata, sizeof(float), width*height*3, fp);
  printf("...done!\n");
  free(imgdata);
  fclose(fp);
  return 0;
}
//======================================================================
// Main
//======================================================================
int main(int argc, const char *argv[])
{
  if (argc < 3) {
    printf("Usage:\texr2pfm <image.exr> <image.pfm>\n");
    return -1;
  }
  int orgWidth, orgHeight;
  bool flipY;
  Array<Rgba> *orgImage = loadEXRFile(argv[1], orgWidth, orgHeight, flipY);
  savePFMFile(argv[2], orgWidth, orgHeight, flipY, orgImage);
  return 0;
}
