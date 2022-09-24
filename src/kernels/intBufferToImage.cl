//
//  intBufferToImage.cl
//  video-mush
//
//  Created by Josh McNamee on 22/08/2014.
//
//

#ifndef video_mush_intBufferToImage_cl
#define video_mush_intBufferToImage_cl

__kernel void charBufferToImage(__global uchar * buffer, write_only image2d_t output, uint width, int bitDepth) {
    const float max = pow(2.0f, (float)bitDepth) - 1.0f;
    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    float out = (float)buffer[coord.y*width+coord.x];
    write_imagef(output, coord, (float4)(out/max, out/max, out/max, 1.0f));
}

__kernel void shortBufferToImage(__global ushort * buffer, write_only image2d_t output, uint width, int bitDepth) {
    const float max = pow(2.0f, (float)bitDepth) - 1.0f;
    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    float out = (float)buffer[coord.y*width+coord.x];
    write_imagef(output, coord, (float4)(out/max, out/max, out/max, 1.0f));
}

#endif
