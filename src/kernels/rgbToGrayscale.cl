//
//  rgbToGrayscle.cl
//
//
//  Created by Josh McNamee on 23/09/2015.
//
//

#ifndef _rgbtograyscale_cl
#define _rgbtograyscale_cl

__kernel void charRGBtoGrayscale(__global char * input, __global char * output) {
    int x = get_global_id(0);
    
    const char in_a = input[x*3];
    
    output[x] = in_a;
}

#endif

