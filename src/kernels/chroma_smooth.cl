//
//  chromaSwap.cl
//  video-mush
//
//  Created by Josh McNamee on 24/03/2015.
//
//

#ifndef video_mush_chromaSmooth_cl
#define video_mush_chromaSmooth_cl

float4 bt709yuvrgb(float4 input);
float4 bt709rgbyuv(float4 input);


float4 chroma_smooth_bilateral(read_only image2d_t input, float sigmaS, float sigmaR, int halfWindow, int2 p) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 size = get_image_dim(input); // Image size
    
    int2 q;;
    
    if (p.x <= size.x && p.y <= size.y) {
        const float4 ip = bt709rgbyuv(read_imagef(input, sampler, p));
        
        float4 sum = 0.0f, norm = 0.0f;
        
        const int2 start = (int2)(p.x - halfWindow, p.y - halfWindow), end = (int2)(p.x + halfWindow, p.y + halfWindow);
        
        for (q.y = start.y; q.y <= end.y; ++q.y) {
            for (q.x = start.x; q.x <= end.x; ++q.x) {
                if (q.x >= 0 && q.y >= 0 && q.x <= size.x && q.y <= size.y) {
                    float4 iq = bt709rgbyuv(read_imagef(input, sampler, q));
                    
                    float2 space = (float2)(p.x - q.x, p.y - q.y);
                    float gs = exp(-sigmaS * (space.x*space.x + space.y*space.y));
                    
                    float4 range = ip - iq;
                    float4 gr = exp(-sigmaR * (range*range));
                    
                    float4 factor = gs * gr;
                    
                    sum += factor * iq;
                    norm += factor;
                }
            }
        }
        
        const float4 out = sum / norm;
        return out;
    }
    
    return (float4)(0.0f, 0.5f, 0.5f, 1.0f);
}

__kernel void chroma_smooth(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int x = get_global_id(0), y = get_global_id(1);
    
    const float4 in_l = bt709rgbyuv(read_imagef(input, sampler, (int2)(x, y)));
    
    const float4 smooth = chroma_smooth_bilateral(input, 0.2f, 2.4f, 5, (int2)(x, y));
    
    const float4 temp = bt709yuvrgb((float4)(in_l.x, smooth.y, smooth.z, 1.0f));
    
    write_imagef(output, (int2)(x, y), temp);
}


void swap( float *a, float *b )
{
    float tmp = *b;
    *b = *a;
    *a = tmp;
}

void sort3( float *a, float *b, float *c )
{
    if( *a > *b )
        swap( a, b );
    if( *b > *c )
        swap( b, c );
    if( *a > *b )
        swap( a, b );
}


__kernel void chroma_median(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    float4 in = bt709rgbyuv(read_imagef(input, sampler, coord) * 8.0f);
    
    int half_size = 1;
    {
        float row1[3], row2[3], row3[3];
        float * rows[3];
        rows[0] = row1;
        rows[1] = row2;
        rows[2] = row3;
        
        for (int j = -half_size; j <= half_size; ++j) {
            for (int i = -half_size; i <= half_size; ++i) {
                rows[j+1][i+1] = bt709rgbyuv(read_imagef(input, sampler, coord + (int2){i, j}) * 8.0f).y;
            }
        }
        
        // sort the rows
        sort3( &(row1[0]), &(row1[1]), &(row1[2]) );
        sort3( &(row2[0]), &(row2[1]), &(row2[2]) );
        sort3( &(row3[0]), &(row3[1]), &(row3[2]) );
        // sort the cols
        sort3( &(row1[0]), &(row2[0]), &(row3[0]) );
        sort3( &(row1[1]), &(row2[1]), &(row3[1]) );
        sort3( &(row1[2]), &(row2[2]), &(row3[2]) );
        // sort the diagonal
        sort3( &(row1[0]), &(row2[1]), &(row3[2]) );
        
        if (fabs(row2[1] - in.y) > 0.1f) {
            in.y = row2[1];
        }
        
    }
    
    {
        float row1[3], row2[3], row3[3];
        float * rows[3];
        rows[0] = row1;
        rows[1] = row2;
        rows[2] = row3;
        
        for (int j = -half_size; j <= half_size; ++j) {
            for (int i = -half_size; i <= half_size; ++i) {
                rows[j+1][i+1] = bt709rgbyuv(read_imagef(input, sampler, coord + (int2){i, j}) * 8.0f).z;
            }
        }
        
        // sort the rows
        sort3( &(row1[0]), &(row1[1]), &(row1[2]) );
        sort3( &(row2[0]), &(row2[1]), &(row2[2]) );
        sort3( &(row3[0]), &(row3[1]), &(row3[2]) );
        // sort the cols
        sort3( &(row1[0]), &(row2[0]), &(row3[0]) );
        sort3( &(row1[1]), &(row2[1]), &(row3[1]) );
        sort3( &(row1[2]), &(row2[2]), &(row3[2]) );
        // sort the diagonal
        sort3( &(row1[0]), &(row2[1]), &(row3[2]) );
        
        if (fabs(row2[1] - in.y) > 0.1f) {
            in.z = row2[1];
        }
    }
    //in.x = 0.5f;
    write_imagef(output, coord, bt709yuvrgb(in) * 0.125f);
}



#endif
