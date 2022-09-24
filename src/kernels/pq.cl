//
//  pq.cl
//  video-mush
//
//  Created by Josh McNamee on 14/01/2015.
//
//

#ifndef video_mush_pq_cl
#define video_mush_pq_cl

float4 bt709yuvrgb(float4 input);

__constant float _pq_c1 = 0.8359375;
__constant float _pq_c2 = 18.8515625;
__constant float _pq_c3 = 18.6875;

__constant float _pq_n = 0.1593017578125;
__constant float _pq_m = 78.8438;

__constant float L = 10000.0;

float pqforward(float in) {
    if (in < 1e-6) {
        return 0.0f;
    }
    const float oneover_n = 1.0/_pq_n;
    const float oneover_m = 1.0/_pq_m;
    
    const float P = pow(in, oneover_m);
    
    const float out = L * pow( (P - _pq_c1) / (_pq_c2 - _pq_c3 * P) , oneover_n);
    return out;
}

float pqbackward(float in) {
    
    const float P = pow(in / L, _pq_n);
    
    const float out = pow( (_pq_c2 * P + _pq_c1) / (1.0f + _pq_c3 * P), _pq_m);
    return out;
}

__kernel void pqencode(read_only image2d_t input, write_only image2d_t output, const float yuvMax) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    // suspect we're being had here, and this is decode
    
    float4 in = read_imagef(input, sampler, (int2)(x, y));
    
    
    /*in.x = pqbackward(clamp(in.x, 0.0f, 1.0f));
    in.y = pqbackward(clamp(in.y, 0.0f, 1.0f));
    in.z = pqbackward(clamp(in.z, 0.0f, 1.0f));*/
    
    in.x = pqbackward(in.x);
    in.y = pqbackward(in.y);
    in.z = pqbackward(in.z);
    
    write_imagef(output, (int2)(x, y), in);
}

__kernel void pqencodeLegacy(read_only image2d_t input, write_only image2d_t output, const float yuvMax) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    float4 in = read_imagef(input, sampler, (int2)(x, y)) / yuvMax;
    
    in = bt709rgbyuv(clamp(in, 0.0f, 1.0f));
    
    in.x = pqbackward(in.x);
    
    write_imagef(output, (int2)(x, y), bt709yuvrgb(in));
}

__kernel void pqdecode(read_only image2d_t input, write_only image2d_t output) {
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    float4 rgb = read_imagef(input, sampler, (int2)(x, y));
    
    rgb.x = pqforward(rgb.x);
    rgb.y = pqforward(rgb.y);
    rgb.z = pqforward(rgb.z);
    
    write_imagef(output, (int2)(x, y), rgb);
    
}

float4 _pqdecode(float in, const float inU, const float inV, const int bitDepth) {
    
    const float bitValues = pow(2.0f, bitDepth) - 1.0f;
    
    const float footRoom = pow(2.0f, bitDepth - 4); // divide by 16;
    const float headRoom = pow(2.0f, bitDepth - 8)*(20.0f); // divide by 256
    
    const float validValues = bitValues - footRoom - headRoom;
    
    const float validChromaValues = bitValues - footRoom - footRoom;
    
    in = (in - footRoom) / validValues;
    in = clamp(in, 0.0f, 1.0f);
    
    const float oU = (inU - footRoom) / validChromaValues;
    const float oV = (inV - footRoom) / validChromaValues;
    
    float4 rgb = bt709yuvrgb((float4)(in, oU, oV, 1.0f));
    rgb.x = pqforward(rgb.x);
    rgb.y = pqforward(rgb.y);
    rgb.z = pqforward(rgb.z);
    
    return rgb;
}

float4 _pqdecodeLegacy(float in, const float inU, const float inV, const int bitDepth) {
    
    const float bitValues = pow(2.0f, bitDepth) - 1.0f;
    
    const float footRoom = pow(2.0f, bitDepth - 4); // divide by 16;
    const float headRoom = pow(2.0f, bitDepth - 8)*(20.0f); // divide by 256
    
    const float validValues = bitValues - footRoom - headRoom;
    
    const float validChromaValues = bitValues - footRoom - footRoom;
    
    in = (in - footRoom) / validValues;
    in = clamp(in, 0.0f, 1.0f);
    
    const float oU = (inU - footRoom) / validChromaValues;
    const float oV = (inV - footRoom) / validChromaValues;
    
    in = pqforward(in);
    float4 rgb = bt709yuvrgb((float4)(in, oU, oV, 1.0f));
    
    return rgb;
}

__kernel void pqdecodeChar(__global uchar * input, write_only image2d_t output, const float yuvMax, int bitDepth) {
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(output);
    
    const float in = (float)input[y*dim.x + x];// yuvMax;
    const float inU = (float)input[(dim.x*dim.y) + (int)(dim.x/2)*y + (int)(x/2)];
    const float inV = (float)input[(int)(dim.x*dim.y*1.5) + (int)(dim.x/2)*y + (int)(x/2)];
    const float4 out = _pqdecode(in, inU, inV, bitDepth);
    
    write_imagef(output, (int2)(x, y), out*yuvMax);
}


__kernel void pqdecodeShort(__global ushort * input, write_only image2d_t output, const float yuvMax, int bitDepth) {
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(output);
    
    const float in = (float)input[y*dim.x + x];// yuvMax;
    const float inU = (float)input[(dim.x*dim.y) + (int)(dim.x/2)*y + (int)(x/2)];
    const float inV = (float)input[(int)(dim.x*dim.y*1.5) + (int)(dim.x/2)*y + (int)(x/2)];
    const float4 out = _pqdecode(in, inU, inV, bitDepth);
    
    write_imagef(output, (int2)(x, y), out*yuvMax);
}


__kernel void pqdecodeCharLegacy(__global uchar * input, write_only image2d_t output, const float yuvMax, int bitDepth) {
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(output);
    
    const float in = (float)input[y*dim.x + x];// yuvMax;
    const float inU = (float)input[(dim.x*dim.y) + (int)(dim.x/2)*y + (int)(x/2)];
    const float inV = (float)input[(int)(dim.x*dim.y*1.5) + (int)(dim.x/2)*y + (int)(x/2)];
    const float4 out = _pqdecodeLegacy(in, inU, inV, bitDepth);
    
    write_imagef(output, (int2)(x, y), out*yuvMax);
}


__kernel void pqdecodeShortLegacy(__global ushort * input, write_only image2d_t output, const float yuvMax, int bitDepth) {
    int x = get_global_id(0), y = get_global_id(1);
    int2 dim = get_image_dim(output);
    
    const float in = (float)input[y*dim.x + x];// yuvMax;
    const float inU = (float)input[(dim.x*dim.y) + (int)(dim.x/2)*y + (int)(x/2)];
    const float inV = (float)input[(int)(dim.x*dim.y*1.5) + (int)(dim.x/2)*y + (int)(x/2)];
    const float4 out = _pqdecodeLegacy(in, inU, inV, bitDepth);
    
    write_imagef(output, (int2)(x, y), out*yuvMax);
}

__kernel void bt709luminance(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 out = bt709rgbyuv(read_imagef(input, sampler, (int2)(x, y)));
    write_imagef(output, (int2)(x, y), (float4)(out.x, out.x, out.x, 1.0f));
}

#endif

