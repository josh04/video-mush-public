//
//  exposure.cl
//  video-mush
//
//  Created by Josh McNamee on 13/06/2014.
//  Copyright (c) 2014. All rights reserved.
//

#ifndef media_encoder_decodemlv_cl
#define media_encoder_decodemlv_cl

float hat(float l);

float dual_hat(float l) {
    return clamp(1.0f - pow(fabs((2.0f * l) - 1.0f), 12.0f), 0.0f, 1.0f);
}

float short_hat(ushort in, ushort max) {
    float in_f = (float)(in)/(float)max;
    return dual_hat(in_f);
}

__kernel void mlv_raw_map(__global ushort * input, __global ushort8 * output) {

    //const int2 coord = (int2)(get_global_id(0), get_global_id(1));
	const int x = get_global_id(0);
    ushort seven[7];
            
	seven[0] = input[x*7];
	seven[1] = input[x*7 + 1];
	seven[2] = input[x*7 + 2];
	seven[3] = input[x*7 + 3];
	seven[4] = input[x*7 + 4];
	seven[5] = input[x*7 + 5];
	seven[6] = input[x*7 + 6];

    ushort8 eight = {0,0,0,0,0,0,0,0};
                
    eight.s0 = seven[0] >> 2;
                
	eight.s1 = seven[0] & 0x0003;
	eight.s1 = eight.s1 << 12;
	eight.s1 = eight.s1 + (seven[1] >> 4);
                
                
	eight.s2 = seven[1] & 0x000F;
	eight.s2 = eight.s2 << 10;
	eight.s2 = eight.s2 + (seven[2] >> 6);
                
	eight.s3 = seven[2] & 0x003F;
	eight.s3 = eight.s3 << 8;
	eight.s3 = eight.s3 + (seven[3] >> 8);
                
                
	eight.s4 = seven[3] & 0x00FF;
	eight.s4 = eight.s4 << 6;
	eight.s4 = eight.s4 + (seven[4] >> 10);
                
	eight.s5 = seven[4] & 0x03FF;
	eight.s5 = eight.s5 << 4;
	eight.s5 = eight.s5 + (seven[5] >> 12);
                
	eight.s6 = seven[5] & 0x0FFF;
	eight.s6 = eight.s6 << 2;
	eight.s6 = eight.s6 + (seven[6] >> 14);
                
	eight.s7 = seven[6] & 0x3FFF;

	output[x] = eight;

}

__kernel void short_buffer_to_image(__global ushort * input, write_only image2d_t output, const uint width, const uint height, const int black, const int white) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    float4 pixel;
    
    int val = (ushort)input[coord.x+coord.y*width];
    val = min(val, (int)white);
    //val = val - (black - 64);
    val = val - black;
    val = max(val, (int)0);
    float div = pow(2.0f, 14)-1.0f;
    float scaled = (float)val / div;
    scaled = scaled / 0.8f; // UPSCAAAALE
    
    //float h = short_hat(val, 15000 - black - 64);
    //float h = dual_hat(scaled);
    
    pixel = (float4)(scaled, scaled, scaled, 1.0f);
    
    write_imagef(output, (int2)(coord.x, coord.y), pixel);
}

__kernel void simple_debayer(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    
    if ((coord.x % 2) == 0) {
        if ((coord.y % 2) == 0) {
            red = read_imagef(input, sampler, coord).x;
            green = (read_imagef(input, sampler, coord + (int2)(1,0)).x + read_imagef(input, sampler, coord + (int2)(0,1)).x) / 2.0f;
            blue = read_imagef(input, sampler, coord + (int2)(1,1)).x;
        } else {
            green = read_imagef(input, sampler, coord).x;
            red = read_imagef(input, sampler, coord - (int2)(1,0)).x;
            blue = read_imagef(input, sampler, coord + (int2)(0,1)).x;
        }
    } else {
        if ((coord.y % 2) == 0) {
            green = read_imagef(input, sampler, coord).x;
            red = read_imagef(input, sampler, coord - (int2)(0,1)).x;
            blue = read_imagef(input, sampler, coord + (int2)(1,0)).x;
        } else {
            blue = read_imagef(input, sampler, coord).x;
            green = (read_imagef(input, sampler, coord - (int2)(1,0)).x + read_imagef(input, sampler, coord - (int2)(0,1)).x) / 2.0f;
            red = read_imagef(input, sampler, coord - (int2)(1,1)).x;
        }
    }
    write_imagef(output, coord, (float4)(red, green, blue, 1.0f));
}

float gaussian_parameter(float scale, float x, float y, float x0, float y0, float dev_x, float dev_y);

float _bicubic_parameter(float x, float a) {
    float ret;
    float abs_x = fabs(x);
    if (abs_x < 3) {
        ret = 0.624f;//(a + 2.0f) * pow(abs_x, 3.0f) - (a + 3.0f) * pow(abs_x, 2.0f) + 1.0f;
    } else if (abs_x < 5) {
        ret = 0.125f;//a * pow(abs_x, 3.0f) - 5.0f * a * pow(abs_x, 2.0f) + 8.0f * a * abs_x - 4.0f * a;
    } else {
        ret = 0.0f;
    }
    return ret;
}

float bicubic_parameter(float x, float y, float a) {
    const float weight_x = _bicubic_parameter(x, a);
    const float weight_y = _bicubic_parameter(y, a);
    return weight_x * weight_y;
}

__kernel void interpolate_green(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    float green = 0.0f;
    const int half_size = 5;
    
    if ((coord.x % 2) == 0) {
        if ((coord.y % 2) == 0) {
            float in_mean = 0.0f;
            float _gauss_sum = 0.0f;
            
            for (int j = -half_size; j <= half_size; ++j) {
                for (int i = -half_size; i <= half_size; ++i) {
                    if (abs(j) % 2 == 1) {
                        if (abs(i) % 2 == 0) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    } else {
                        if (abs(i) % 2 == 1) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    }
                }
            }
            
            in_mean = in_mean / _gauss_sum;
            
            green = in_mean;//(i1 + i2 + i3 + i4) / div;
            //green = _gauss_sum;
            //green = (read_imagef(input, sampler, coord+(int2)(0,-1)).x + read_imagef(input, sampler, coord+(int2)(-1,0)).x + read_imagef(input, sampler, coord+(int2)(1,0)).x + read_imagef(input, sampler, coord+(int2)(0,1)).x) / 4.0f;
        } else {
            green = read_imagef(input, sampler, coord).x;
        }
    } else {
        if ((coord.y % 2) == 0) {
            green = read_imagef(input, sampler, coord).x;
        } else {
            float in_mean = 0.0f;
            float _gauss_sum = 0.0f;
            
            for (int j = -half_size; j <= half_size; ++j) {
                for (int i = -half_size; i <= half_size; ++i) {
                    if (abs(j) % 2 == 1) {
                        if (abs(i) % 2 == 0) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    } else {
                        if (abs(i) % 2 == 1) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    }
                }
            }
            
            in_mean = in_mean / _gauss_sum;
            
            
            green = in_mean;//(i1 + i2 + i3 + i4) / div;
            //green = _gauss_sum;
            //green = read_imagef(input, sampler, coord+(int2)(1,0)).x;
            //green = (read_imagef(input, sampler, coord+(int2)(0,-1)).x + read_imagef(input, sampler, coord+(int2)(-1,0)).x + read_imagef(input, sampler, coord+(int2)(1,0)).x + read_imagef(input, sampler, coord+(int2)(0,1)).x) / 4.0f;
        }
    }
    
    write_imagef(output, coord, (float4)(0.0f, green, 0.0f, 1.0f));
}


__kernel void interpolate_red(read_only image2d_t input, read_only image2d_t output_read, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    float red = 0.0f;
    const int half_size = 5;
    
    if ((coord.x % 2) == 0) {
        if ((coord.y % 2) == 0) {
            red = read_imagef(input, sampler, coord).x;
        } else {
            
            float in_mean = 0.0f;
            float _gauss_sum = 0.0f;
            for (int j = -half_size; j <= half_size; ++j) {
                for (int i = -half_size; i <= half_size; ++i) {
                    if (abs(j) % 2 == 1) {
                        if (abs(i) % 2 == 0) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    }
                }
            }
            
            in_mean = in_mean / _gauss_sum;
            red = in_mean;
        }
    } else {
        if ((coord.y % 2) == 0) {
            float in_mean = 0.0f;
            float _gauss_sum = 0.0f;
            
            for (int j = -half_size; j <= half_size; ++j) {
                for (int i = -half_size; i <= half_size; ++i) {
                    if (abs(j) % 2 == 0) {
                        if (abs(i) % 2 == 1) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    }
                }
            }
            
            in_mean = in_mean / _gauss_sum;
            red = in_mean;
        } else {
            
            float in_mean = 0.0f;
            float _gauss_sum = 0.0f;
            
            for (int j = -half_size; j <= half_size; ++j) {
                for (int i = -half_size; i <= half_size; ++i) {
                    if (abs(j) % 2 == 1) {
                        if (abs(i) % 2 == 1) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    }
                }
            }
            
            red = in_mean / _gauss_sum;
        }
    }
    const float4 current = read_imagef(output_read, sampler, coord);
    write_imagef(output, coord, (float4)(red, current.y, 0.0f, 1.0f));
}

__kernel void interpolate_blue(read_only image2d_t input, read_only image2d_t output_read, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    float blue = 0.0f;
    const int half_size = 5;
    
    if ((coord.x % 2) == 0) {
        if ((coord.y % 2) == 0) {
            float in_mean = 0.0f;
            float _gauss_sum = 0.0f;
            
            for (int j = -half_size; j <= half_size; ++j) {
                for (int i = -half_size; i <= half_size; ++i) {
                    if (abs(j) % 2 == 1) {
                        if (abs(i) % 2 == 1) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    }
                }
            }
            
            in_mean = in_mean / _gauss_sum;
            
            blue = in_mean;
        } else {
            float in_mean = 0.0f;
            float _gauss_sum = 0.0f;
            
            for (int j = -half_size; j <= half_size; ++j) {
                for (int i = -half_size; i <= half_size; ++i) {
                    if (abs(j) % 2 == 0) {
                        if (abs(i) % 2 == 1) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    }
                }
            }
            
            in_mean = in_mean / _gauss_sum;
            blue = in_mean;
        }
    } else {
        if ((coord.y % 2) == 0) {
            float in_mean = 0.0f;
            float _gauss_sum = 0.0f;
            
            for (int j = -half_size; j <= half_size; ++j) {
                for (int i = -half_size; i <= half_size; ++i) {
                    if (abs(j) % 2 == 1) {
                        if (abs(i) % 2 == 0) {
                            const float in_a = read_imagef(input, sampler, coord + (int2){i, j}).x;
                            const float weight = bicubic_parameter(i, j, -0.5f);
                            _gauss_sum = _gauss_sum + weight;
                            in_mean = in_mean + weight * in_a;
                        }
                    }
                }
            }
            
            in_mean = in_mean / _gauss_sum;
            blue = in_mean;
        } else {
            blue = read_imagef(input, sampler, coord).x;
        }
    }
    const float4 current = read_imagef(output_read, sampler, coord);
    write_imagef(output, coord, (float4)(current.x, current.y, blue, 1.0f));
}

float3 xyztorgb(float3 xyz);

__kernel void white_point(read_only image2d_t input, write_only image2d_t output, float4 point, const float4 col1, const float4 col2, const float4 col3, int dual_iso, float cla) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    float4 in = read_imagef(input, sampler, coord);
    
    point = (float4)(point.x / point.y, 1.0f, point.z / point.y, 1.0f);
    
    in = in * point;
    //in = min(in, 32.0f); // RAW clamp, disabled for dual iso
    //in = min(in, cla); // RAW clamp, disabled for dual iso
    
    if (!dual_iso) {
        in = min(in, 1.0f/0.6f); // RAW clamp, disabled for dual 
		in = min(in, cla); // RAW clamp, disabled for dual iso
    }
    /*
	if (in.x > 1.0f) {
		in.y = in.x;
		in.z = in.x;
	}
	
	if (in.z > 1.0f) {
		in.x = in.z;
		in.y = in.z;
	}
	*/
    // MK 2
    
    // { 8924, 10000, -1041, 10000, 1760, 10000,
    //   4351, 10000,  6621, 10000, -972, 10000,
    //    505, 10000, -1562, 10000, 9308, 10000 }
    /*
    if (camera_type == 0) {
        const float4 col1 = (float4)(0.8924, -0.1041,  0.1760, 0.0f);
        const float4 col2 = (float4)(0.4351,  0.6621, -0.0972, 0.0f);
        const float4 col3 = (float4)(0.0505, -0.1562,  0.9308, 0.0f);
        
        in = (float4)(dot(col1, in), dot(col2, in), dot(col3, in), 1.0f);
    } else {
    */
    
    // no idea what these are - 7/8/2016
    /*const float4 col1 = (float4)( 0.8924,  0.4351,  0.0505, 0.0f);
    const float4 col2 = (float4)(-0.1041,  0.6621, -0.1562, 0.0f);
    const float4 col3 = (float4)( 0.1760, -0.0972,  0.9308, 0.0f);*/
    
    //MK 3
    
    //{ 7868, 10000,    92, 10000,  1683, 10000,
    //  2291, 10000,  8615, 10000,  -906, 10000,
    //    27, 10000, -4752, 10000, 12976, 10000 },
    //{ 7637, 10000, 805, 10000, 1201, 10000, 2649, 10000, 9179, 10000, -1828, 10000, 137, 10000, -2456, 10000, 10570, 10000 }
    /*
        const float4 col1 = (float4)(0.7868,  0.0092,  0.1683, 0.0f);
        const float4 col2 = (float4)(0.2291,  0.8615, -0.0906, 0.0f);
        const float4 col3 = (float4)(0.0027, -0.4752,  1.2976, 0.0f);
    */
        in = (float4)(dot(col1, in), dot(col2, in), dot(col3, in), 1.0f);
    //}
    
    float3 in_rgb = xyztorgb((float3)(in.x, in.y, in.z));
    
    in = (float4)(in_rgb.x, in_rgb.y, in_rgb.z, 1.0f);
    write_imagef(output, coord, in);
}

__kernel void quarter_image(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    int2 out_coord;
    
    const float4 in = read_imagef(input, sampler, coord);
    float4 out;
    
    if ((coord.x % 2) == 0) {
        if ((coord.y % 2) == 0) {
            out_coord.x = coord.x/2;
            out_coord.y = coord.y/2;
            out = (float4)(in.x, in.x, in.x, 1.0f);
        } else {
            out_coord.x = coord.x/2 + dim.x/2;
            out_coord.y = coord.y/2;
            out = (float4)(in.x, in.x, in.x, 1.0f);
        }
    } else {
        if ((coord.y % 2) == 0) {
            out_coord.x = coord.x/2;
            out_coord.y = coord.y/2 + dim.y/2;
            out = (float4)(in.x, in.x, in.x, 1.0f);
        } else {
            out_coord.x = coord.x/2 + dim.x/2;
            out_coord.y = coord.y/2 + dim.y/2;
            out = (float4)(in.x, in.x, in.x, 1.0f);
        }
    }
    write_imagef(output, out_coord, out);
}



float light_weight(float l) {
    float ret = 1.0f;
    //ret = 0.0f;
    if (l > 0.97f) {
        ret = 0.0f;
    } else if (l > 0.90f) {
        ret = 1.0f - l; //0.01f;
    } else if (l > 0.7f) {
        
        float num = l - 0.7f;
        float num2 = num * (0.9f / 0.2f);
        
        ret = 1.0f - num2;
    }
    return ret;
}


float dark_weight(float l) {
    float ret = 1.0f;
    //ret = 0.0f;
    if (l < 0.05f) {
        ret = l; //0.01f;
    } else if (l < 0.1f) {
        //ret = 0.2f;
        
        float num2 = (l - 0.05f) * (0.95f / 0.05f);
        
        ret = 0.05f + num2;
        
        //ret = l;
    }
    return ret;
}

float light_weight_red(float l) {
    float ret = 1.0f;
    //ret = 0.0f;
    if (l > 0.97f) {
        ret = 0.0f;
    } else if (l > 0.90f) {
        ret = 1.0f - l; //0.01f;;
    } else if (l > 0.5f) {
        //ret = 0.1f - (l - 0.8f); //ret = 0.1f;
        
        float num = l - 0.5f;
        float num2 = num * (0.9f / 0.4f);
        
        ret = 1.0f - num2;
    }
    return ret;
}

float dark_weight_red(float l) {
    float ret = 1.0f;
    //ret = 0.0f;
    if (l < 0.05f) {
        ret = l; //0.01f; FIXME: this makes no sense
    } else if (l < 0.25f) {
        //ret = l; //0.2f;
        
        float num2 = (l - 0.05f) * (0.95f / 0.2f);
        
        ret = 0.05f + num2;
    }
    return ret;
}

__kernel void dualiso_weight(read_only image2d_t input, write_only image2d_t output, uchar mod, const float red_clamp, const float blue_clamp) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    int2 o_coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    const int2 dim = get_image_dim(output);
    
    float4 in = read_imagef(input, sampler, coord + (int2)(0,0));
    float4 in2 = read_imagef(input, sampler, coord + (int2)(1,0));
    float4 in3 = read_imagef(input, sampler, coord + (int2)(0,1));
    float4 in4 = read_imagef(input, sampler, coord + (int2)(1,1));
    
    in2 = min(in2, red_clamp);
    in3 = min(in3, blue_clamp);
    
    if (((get_global_id(1) + mod) % 2) == 0) {
        
        // LIGHT FRAME
        
        in.s3 = light_weight_red(in.s0);
        
        in2.s3 = light_weight(in2.s0);
        
        in3.s3 = light_weight(in3.s0);
        
        in4.s3 = light_weight_red(in4.s0);
        
        in = min(in, 1.0f);
        in2 = min(in2, 1.0f);
        in3 = min(in3, 1.0f);
        in4 = min(in4, 1.0f);
        
    } else {
        
        // DARK FRAME
        
        in.s3 = dark_weight_red(in.s0);
        
        in2.s3 = dark_weight(in2.s0);
        
        in3.s3 = dark_weight(in3.s0);
        
        in4.s3 = dark_weight_red(in4.s0);
        in = min(in, 1.0f);
        //in2 = pow(min(in2, 1.0f), 0.95f);//*0.6f;
        //in3 = pow(min(in3, 1.0f), 0.85f);//*0.8f;
        in2 = min(in2, 1.0f);
        in3 = min(in3, 1.0f);
        in4 = min(in4, 1.0f);
    }
    /*
    in = (float4)(in.s3, in.s3, in.s3, 1.0f);
    in2 = (float4)(in2.s3, in2.s3, in2.s3, 1.0f);
    in3 = (float4)(in3.s3, in3.s3, in3.s3, 1.0f);
    in4 = (float4)(in4.s3, in4.s3, in4.s3, 1.0f);
    */
    write_imagef(output, o_coord + (int2)(0,0), in);
    write_imagef(output, o_coord + (int2)(1,0), in2);
    write_imagef(output, o_coord + (int2)(0,1), in3);
    write_imagef(output, o_coord + (int2)(1,1), in4);
}


__kernel void weight_display(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    //const int2 dim = get_image_dim(output);
    
    float4 in = read_imagef(input, sampler, coord);
    
    write_imagef(output, coord, (float4)(in.s3, in.s3, in.s3, 1.0f));
}



__kernel void dualiso_alpha_gaussian(read_only image2d_t input, write_only image2d_t output) {
    const float sigma = 2.0f;
    const int half_window = 3;
    
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    int x = get_global_id(0), y = get_global_id(1);
    
    float out_r = 0.0f;
    
    float div = 0.0f;
    
    const float4 in = read_imagef(input, sampler, (int2)(x, y));
    
    const float mult = 1.0f/sqrt(M_PI * 2.0f * pow(sigma, 2.0f));
    const float denom = 2*pow(sigma, 2.0f);
    
    for (int j = -half_window; j < half_window; ++j) {
        for (int i = -half_window; i < half_window; ++i) {
        const float in_r = read_imagef(input, sampler, (int2)(x+i, y+j)).s3;
            
        const float gauss_i = mult * exp(-(pow(i, 2.0f) / denom));
        const float gauss_j = mult * exp(-(pow(j, 2.0f) / denom));
        
        div += gauss_i * gauss_j;
        
        out_r += gauss_i * gauss_j * in_r;
        }
    }
    
    out_r = out_r / div;
    
    write_imagef(output, (int2)(x, y), (float4)(in.x, in.y, in.z, out_r));
    
}

__kernel void dualiso_upsize(read_only image2d_t input, write_only image2d_t output, uchar mod) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    //const int2 o_coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    const int2 dim = get_image_dim(output);
    
    float4 in = (float4)(1.0f, 0.0f, 0.0f, 1.0f);
    float4 in2 = (float4)(1.0f, 0.0f, 0.0f, 1.0f);
    float4 in3 = (float4)(1.0f, 0.0f, 0.0f, 1.0f);
    float4 in4 = (float4)(1.0f, 0.0f, 0.0f, 1.0f);
    
    if (((get_global_id(1) + mod) % 2) == 0) {
        in = read_imagef(input, sampler, coord + (int2)(0,0));
        in2 = read_imagef(input, sampler, coord + (int2)(1,0));
        in3 = read_imagef(input, sampler, coord + (int2)(0,1));
        in4 = read_imagef(input, sampler, coord + (int2)(1,1));
        
        //in2.s3 = 1.0f;
        //in3.s3 = 1.0f;
    } else {
        /* // josh
        const float4 in1_1 =   log(read_imagef(input, sampler, coord + (int2)(0,-2)));
        
        //const float4 in2_1 =   log(read_imagef(input, sampler, coord + (int2)(1,-2)));
        const float4 in2_1_1 = log(read_imagef(input, sampler, coord + (int2)(0,-1)));
        const float4 in2_1_2 = log(read_imagef(input, sampler, coord + (int2)(2,-1)));
        
        const float4 in4_1 =   log(read_imagef(input, sampler, coord + (int2)(1,-1)));
        
        //const float4 in3_1 = read_imagef(input, sampler, coord + (int2)(0,-1));
        
        const float4 in1_2 =   log(read_imagef(input, sampler, coord + (int2)(0,2)));
        
        //const float4 in3_2 =   log(read_imagef(input, sampler, coord + (int2)(0,3)));
        const float4 in3_2_1 = log(read_imagef(input, sampler, coord + (int2)(1,2)));
        const float4 in3_2_2 = log(read_imagef(input, sampler, coord + (int2)(-1,2)));
        
        const float4 in4_2 =   log(read_imagef(input, sampler, coord + (int2)(1,3)));
        
        //const float4 in2_2 = read_imagef(input, sampler, coord + (int2)(1,2));
        
        /*
        in =  (in1_1 + in_2) / 2.0f;
        in2 = (in2_1 + in2_2) / 2.0f;
        in3 = (in3_1 + in3_2) / 2.0f;
        in4 = (in4_1 + in4_2) / 2.0f;
        *
        
        in =  exp((in1_1 + in1_2) / 2.0f);
        const float4 t_in2 = (in2_1_1 + in2_1_2) / 2.0f;
        const float4 t_in3 = (in3_2_1 + in3_2_2) / 2.0f;
        in2 = exp(0.6667f * t_in2 + 0.3333f * t_in3);
        in3 = exp(0.3333f * t_in2 + 0.6667f * t_in3);
        in4 = exp((in4_1 + in4_2) / 2.0f);
        
        */ // josh
        
        
        //in = exp(in1_1);
        //in2 = (in2_1_1);
        //in2 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
        //in2.s3 = 1.0f;
        //in3 = (in3_2_1);
        //in3.s3 = 1.0f;
        
        //in4 = exp(in4_1);
        
        float in_mean = 0.0f;
        float _gauss_sum_r_b = 0.0f, _gauss_sum_g1 = 0.0f, _gauss_sum_g2 = 0.0f;
        
        int half_size = 2;
        
        float4 r = (float4)(0.0f, 0.0f, 0.0f, 0.0f), g1 = (float4)(0.0f, 0.0f, 0.0f, 0.0f), g2 = (float4)(0.0f, 0.0f, 0.0f, 0.0f), b = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
        
        for (int j = -half_size; j <= half_size; j++) {
            
            if (abs(j) % 4 == 2) {
                
                for (int i = -half_size; i <= half_size; i = i + 2) {
                    //if (abs(i) % 2 == 0) {
                        const float4 in_r = read_imagef(input, sampler, coord + (int2){i, j});
                        const float4 in_g1 = read_imagef(input, sampler, coord + (int2){i+1, j});
                        const float4 in_g2 = read_imagef(input, sampler, coord + (int2){i, j+1});
                        const float4 in_b = read_imagef(input, sampler, coord + (int2){i+1, j+1});
                    
                        const float weight_r_b = bicubic_parameter(i, j, -0.5f);
                        
                        const float weight_g1 = bicubic_parameter(i-1, j-1, -0.5f);
                        const float weight_g2 = bicubic_parameter(i+1, j+1, -0.5f);
                        
                        _gauss_sum_r_b = _gauss_sum_r_b + weight_r_b;
                        _gauss_sum_g1 = _gauss_sum_g1 + weight_r_b + weight_g2;
                        _gauss_sum_g2 = _gauss_sum_g2 + weight_r_b + weight_g1;
                    
                    
                        r = r + weight_r_b * in_r;
                        g1 = g1 + weight_r_b * in_g1 + in_g2 * weight_g2;
                        g2 = g2 + weight_r_b * in_g2 + in_g1 * weight_g1;
                        b = b + weight_r_b * in_b;
                    //}
                }
                
            }
            
        }
        
        r = fabs(r) / (_gauss_sum_r_b);
        g1 = fabs(g1) / (_gauss_sum_g1);
        g2 = fabs(g2) / (_gauss_sum_g2);
        b = fabs(b) / (_gauss_sum_r_b);
        
        
        r.s3 = r.s3 * 0.5f;
        g1.s3 = g1.s3 * 0.5f;
        g2.s3 = g2.s3 * 0.5f;
        b.s3 = b.s3 * 0.5f;
        
        
        in = r;
        in2 = g1;
        in3 = g2;
        in4 = b;
        
    }
    /*
    in = (float4)(in.s3, in.s3, in.s3, 1.0f);
    in2 = (float4)(in2.s3, in2.s3, in2.s3, 1.0f);
    in3 = (float4)(in3.s3, in3.s3, in3.s3, 1.0f);
    in4 = (float4)(in4.s3, in4.s3, in4.s3, 1.0f);*/
    //in = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    //in4 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    write_imagef(output, coord + (int2)(0,0), in);//(float4)(in.w, in.w, in.w, 0.0f));
    write_imagef(output, coord + (int2)(1,0), in2);//(float4)(in2.w, in2.w, in2.w, 0.0f));
    write_imagef(output, coord + (int2)(0,1), in3);//(float4)(in3.w, in3.w, in3.w, 0.0f));
    write_imagef(output, coord + (int2)(1,1), in4);//(float4)(in4.w, in4.w, in4.w, 0.0f));
}


__kernel void dualiso_linear_upsize(read_only image2d_t input, write_only image2d_t output, uchar mod) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    //const int2 o_coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    const int2 dim = get_image_dim(output);
    
    float4 in = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 in2 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 in3 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    float4 in4 = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    if (((get_global_id(1) + mod) % 2) == 0) {
        in = read_imagef(input, sampler, coord + (int2)(0,0));
        in2 = read_imagef(input, sampler, coord + (int2)(1,0));
        in3 = read_imagef(input, sampler, coord + (int2)(0,1));
        in4 = read_imagef(input, sampler, coord + (int2)(1,1));
        /*
        in += (float4)(0.0f, 0.0f, 0.1f, 0.0f);
        in2 += (float4)(0.1f, 0.0f, 0.1f, 0.0f);
        in3 += (float4)(0.1f, 0.0f, 0.1f, 0.0f);
        in4 += (float4)(0.1f, 0.0f, 0.0f, 0.0f);
         */
    } else {

        float in_mean = 0.0f;
        float _gauss_sum_r_b = 0.0f, _gauss_sum_g1 = 0.0f, _gauss_sum_g2 = 0.0f;
        
        int half_size = 2;
        
        float4 r = (float4)(0.0f, 0.0f, 0.0f, 0.0f),
        g1 = (float4)(0.0f, 0.0f, 0.0f, 0.0f),
        g2 = (float4)(0.0f, 0.0f, 0.0f, 0.0f),
        b = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
        /*
        for (int j = -half_size; j <= half_size; j++) {
            
            if (abs(j) % 4 == 2) {
                
                for (int i = -half_size; i <= half_size; i = i + 2) {
                    //if (abs(i) % 2 == 0) {
                    const float4 in_r = read_imagef(input, sampler, coord + (int2){i, j});
                    const float4 in_b = read_imagef(input, sampler, coord + (int2){i+1, j+1});
                    
                    const float weight_r_b = bicubic_parameter(i, j, -0.5f);
                    
                    
                    _gauss_sum_r_b = _gauss_sum_r_b + weight_r_b;
                    
                    
                    r = r + weight_r_b * in_r;
                    b = b + weight_r_b * in_b;
                    //}
                }
                
            }
            
        }
         
        
        r = fabs(r) / (_gauss_sum_r_b);
        b = fabs(b) / (_gauss_sum_r_b);
        */
        
        float4 r_1 = read_imagef(input, sampler, coord + (int2)(0,2));
        float4 r_2 = read_imagef(input, sampler, coord + (int2)(0,-2));
        r = r_1  * 0.5f + r_2 * 0.5f;
        float4 b_1 = read_imagef(input, sampler, coord + (int2)(1,-1));
        float4 b_2 = read_imagef(input, sampler, coord + (int2)(1, 3));
        b = b_1  * 0.5f + b_2 * 0.5f;
        //b.s0 = 100.0f;
        
        r = r * 0.80f + b * 0.20f;
        b = r * 0.20f + b * 0.80f;
        
        in = r;
        in4 = b;
        
        
        float4 in_g1_1 = read_imagef(input, sampler, coord + (int2)(0,-1));
        float4 in_g1_2 = read_imagef(input, sampler, coord + (int2)(2,-1));
        
        g1 = in_g1_1 * 0.5f + in_g1_2 * 0.5f;
        
        float4 in_g2_1 = read_imagef(input, sampler, coord + (int2)(-1,2));
        float4 in_g2_2 = read_imagef(input, sampler, coord + (int2)(1,2));
        
        g2 = in_g2_1 * 0.5f + in_g2_2 * 0.5f;
        
        g1 = g1 * 0.60f + g2 * 0.40f;
        g2 = g1 * 0.40f + g2 * 0.60f;
        
        r = 0.6f * r + 0.4f * g1;
        b = 0.6f * b + 0.4f * g2;
        
        //g1 = g1 * 0.60f + r * 0.20f + b * 0.20f;
        //g2 = g2 * 0.60f + r * 0.20f + b * 0.20f;
        
        in2 = g2;
        in3 = g1;
        /*
        in += (float4)(0.0f, 0.0f, 0.1f, 0.0f);
        in2 += (float4)(0.1f, 0.0f, 0.1f, 0.0f);
        in3 += (float4)(0.1f, 0.0f, 0.1f, 0.0f);
        in4 += (float4)(0.1f, 0.0f, 0.0f, 0.0f);
        */
        
        
        in.s3 = r.s3 * 0.5f;
        in2.s3 = g1.s3 * 0.5f;
        in3.s3 = g2.s3 * 0.5f;
        in4.s3 = b.s3 * 0.5f;
        
        
    }
    
    write_imagef(output, coord + (int2)(0,0), in);//(float4)(in.w, in.w, in.w, 0.0f));
    write_imagef(output, coord + (int2)(1,0), in2);//(float4)(in2.w, in2.w, in2.w, 0.0f));
    write_imagef(output, coord + (int2)(0,1), in3);//(float4)(in3.w, in3.w, in3.w, 0.0f));
    write_imagef(output, coord + (int2)(1,1), in4);//(float4)(in4.w, in4.w, in4.w, 0.0f));
}


float dualiso_hat(float l) {
    float out = 1.0f;
    if (l > 0.95) {
        out = 0.0f;
    }
    return out;
}

float4 dualiso_scale_balance(const float4 a, const float4 b, const float4 b2, const float weight1, const float weight2, const float fac, const float hat_a, const float hat_b, const float hat_b2) {
    
    
    float4 in =  hat_a * weight1 * a * fac + hat_b * weight2 * b + hat_b2 * weight2 * b2;
    in = in / (hat_a * weight1 + hat_b * weight2 + hat_b2 * weight2);
    return in;
}

__kernel void dualiso_scale(read_only image2d_t input, write_only image2d_t output, uchar mod, float fac) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    int2 o_coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    const int2 dim = get_image_dim(output);
    
    float4 in = read_imagef(input, sampler, coord + (int2)(0,0));
    float4 in2 = read_imagef(input, sampler, coord + (int2)(1,0));
    float4 in3 = read_imagef(input, sampler, coord + (int2)(0,1));
    float4 in4 = read_imagef(input, sampler, coord + (int2)(1,1));
    
    if (((get_global_id(1) + mod) % 2) == 0) {
        float alph;
        
        alph = in.s3;
        in = in * fac;
        in.s3 = alph;
        
        alph = in2.s3;
        in2 = in2 * fac;
        in2.s3 = alph;
        
        alph = in3.s3;
        in3 = in3 * fac;
        in3.s3 = alph;
        
        alph = in4.s3;
        in4 = in4 * fac;
        in4.s3 = alph;
        
        //o_coord = (int2)(get_global_id(0)*2, get_global_id(1) + dim.y/2 - 1);
    } else {
        
        //o_coord = (int2)(get_global_id(0)*2, get_global_id(1)+1);
        /*in = (float4)(1.0f, 1.0f, 1.0f, 1.0f);
        in2 = (float4)(1.0f, 1.0f, 1.0f, 1.0f);
        in3 = (float4)(1.0f, 1.0f, 1.0f, 1.0f);
        in4 = (float4)(1.0f, 1.0f, 1.0f, 1.0f);*/
    }
    
    write_imagef(output, o_coord + (int2)(0,0), in);//(float4)(in.x, in.y, in.z, 1.0f));
    write_imagef(output, o_coord + (int2)(1,0), in2);//(float4)(in2.x, in2.y, in2.z, 1.0f));
    write_imagef(output, o_coord + (int2)(0,1), in3);//(float4)(in3.x, in3.y, in3.z, 1.0f));
    write_imagef(output, o_coord + (int2)(1,1), in4);//(float4)(in4.x, in4.y, in4.z, 1.0f));
}

__kernel void dualiso_merge(write_only image2d_t output, read_only image2d_t upsize_dark, read_only image2d_t upsize_light, read_only image2d_t scale_light, float fac) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 o_coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    const float min_alph = 0.01f;
    
    const float4 up_d = read_imagef(upsize_dark, sampler, coord);
    const float up_d_alph = max(up_d.s3, min_alph);
    
    const float4 up_l = read_imagef(upsize_light, sampler, coord);
    const float up_l_alph = max(up_l.s3, min_alph);
    
    const float4 sc_l = read_imagef(scale_light, sampler, coord);
    const float sc_l_alph = max(sc_l.s3, min_alph);
    
    
    const float div = up_d_alph + up_l_alph + sc_l_alph;
    const float4 out = (up_d * up_d_alph
                      + (up_l) * up_l_alph
                      + (sc_l) * sc_l_alph
                        )
                        / (div);
    
    //const float4 out = (up_d + (up_l/fac) + sc_d + (sc_l/fac))/4.0f;
    //const float4 out = up_d;
    
    write_imagef(output, o_coord, (float4)(out.x, out.y, out.z, 1.0f));
}

__kernel void dualiso_merge2(write_only image2d_t output, read_only image2d_t upsize_dark, read_only image2d_t upsize_light, read_only image2d_t scale_light, float fac) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    const int2 o_coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    const int2 dim = get_image_dim(output);
    const float min_alph = 0.0001f;
    
    const float4 up_d = read_imagef(upsize_dark, sampler, coord+(int2)(0,0));
    const float up_d_alph = max(up_d.s3, min_alph);
    const float4 up_d2 = read_imagef(upsize_dark, sampler, coord+(int2)(1,0));
    const float up_d2_alph = max(up_d2.s3, min_alph);
    const float4 up_d3 = read_imagef(upsize_dark, sampler, coord+(int2)(0,1));
    const float up_d3_alph = max(up_d3.s3, min_alph);
    const float4 up_d4 = read_imagef(upsize_dark, sampler, coord+(int2)(1,1));
    const float up_d4_alph = max(up_d4.s3, min_alph);
    
    const float4 up_l = read_imagef(upsize_light, sampler, coord+(int2)(0,0));
    const float up_l_alph = max(up_l.s3, min_alph);
    const float4 up_l2 = read_imagef(upsize_light, sampler, coord+(int2)(1,0));
    const float up_l2_alph = max(up_l2.s3, min_alph);
    const float4 up_l3 = read_imagef(upsize_light, sampler, coord+(int2)(0,1));
    const float up_l3_alph = max(up_l3.s3, min_alph);
    const float4 up_l4 = read_imagef(upsize_light, sampler, coord+(int2)(1,1));
    const float up_l4_alph = max(up_l4.s3, min_alph);
    
    const float4 sc_l = read_imagef(scale_light, sampler, coord+(int2)(0,0));
    const float sc_l_alph = max(sc_l.s3, min_alph);
    
    const float4 sc_l2 = read_imagef(scale_light, sampler, coord+(int2)(1,0));
    const float sc_l2_alph = max(sc_l2.s3, min_alph);
    
    const float4 sc_l3 = read_imagef(scale_light, sampler, coord+(int2)(0,1));
    const float sc_l3_alph = max(sc_l3.s3, min_alph);
    
    const float4 sc_l4 = read_imagef(scale_light, sampler, coord+(int2)(1,1));
    const float sc_l4_alph = max(sc_l4.s3, min_alph);
    
    const float up_d_alph_fin = min(min(min(up_d_alph, up_d2_alph), up_d3_alph), up_d4_alph);
    
    const float up_l_alph_fin = min(min(min(up_l_alph, up_l2_alph), up_l3_alph), up_l4_alph);
    
    const float sc_l_alph_fin = min(min(min(sc_l_alph, sc_l2_alph), sc_l3_alph), sc_l4_alph);
    
    
    const float div = up_d_alph_fin + up_l_alph_fin + sc_l_alph_fin;
    const float4 out = (up_d * up_d_alph_fin
                        + (up_l /*/ fac*/) * up_l_alph_fin
                        + (sc_l /*/ fac*/) * sc_l_alph_fin
                        )
    / (div);
    const float4 out2 = (up_d2 * up_d_alph_fin
                         + (up_l2 /*/ fac*/) * up_l_alph_fin
                         + (sc_l2 /*/ fac*/) * sc_l_alph_fin
                         )
    / (div);
    const float4 out3 = (up_d3 * up_d_alph_fin
                         + (up_l3 /*/ fac*/) * up_l_alph_fin
                         + (sc_l3 /*/ fac*/) * sc_l_alph_fin
                         )
    / (div);
    const float4 out4 = (up_d4 * up_d_alph_fin
                         + (up_l4 /*/ fac*/) * up_l_alph_fin
                         + (sc_l4 /*/ fac*/) * sc_l_alph_fin
                         )
    / (div);
    //const float4 out = (up_d + (up_l/fac) + sc_d + (sc_l/fac))/4.0f;
    //const float4 out = up_d;
    
    write_imagef(output, o_coord+(int2)(0,0), (float4)(out.x, out.y, out.z, 1.0f));
    write_imagef(output, o_coord+(int2)(1,0), (float4)(out2.x, out2.y, out2.z, 1.0f));
    write_imagef(output, o_coord+(int2)(0,1), (float4)(out3.x, out3.y, out3.z, 1.0f));
    write_imagef(output, o_coord+(int2)(1,1), (float4)(out4.x, out4.y, out4.z, 1.0f));
}

__kernel void dualiso_neaten(read_only image2d_t input, write_only image2d_t output, uchar mod, float fac) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    int2 o_coord = (int2)(get_global_id(0)*2, get_global_id(1)*2);
    const int2 dim = get_image_dim(output);
    
    float4 in = read_imagef(input, sampler, coord + (int2)(0,0));
    float4 in2 = read_imagef(input, sampler, coord + (int2)(1,0));
    float4 in3 = read_imagef(input, sampler, coord + (int2)(0,1));
    float4 in4 = read_imagef(input, sampler, coord + (int2)(1,1));
    /*
    if (((get_global_id(1) + mod) % 2) == 0) {
        in = in * 1.0f;
        in2 = in2 * 1.0f;
        in3 = in3 * 1.0f;
        in4 = in4 * 1.0f;
    }*/
    /*
    if (in.x > 0.99 && in4.x > 0.99) {
        in2 = in2 * 4.0f;
        in3 = in3 * 4.0f;
    } else if (in.x > 0.99) {
        in2 = in2 * 4.0f;
        in3 = in3 * 4.0f;
        in4 = in4 * 3.0f;
    } else if (in4.x > 0.99) {
        in = in * 1.25f;
        in2 = in2 * 2.0f;
        in3 = in3 * 2.0f;
    }*/
	
    write_imagef(output, o_coord + (int2)(0,0), in);//(float4)(in.x, in.y, in.z, 1.0f));
    write_imagef(output, o_coord + (int2)(1,0), in2);//(float4)(in2.x, in2.y, in2.z, 1.0f));
    write_imagef(output, o_coord + (int2)(0,1), in3);//(float4)(in3.x, in3.y, in3.z, 1.0f));
    write_imagef(output, o_coord + (int2)(1,1), in4);//(float4)(in4.x, in4.y, in4.z, 1.0f));
}

__kernel void dualiso_quarter(write_only image2d_t output, read_only image2d_t upsize_dark, read_only image2d_t upsize_light, read_only image2d_t scale_light, float fac) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    float4 out = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    if (coord.x < dim.x/2) {
        if (coord.y < dim.y/2) {
            const int2 i_coord = (int2)(coord.x/2, coord.y/2);
            out = read_imagef(upsize_dark, sampler, i_coord);
        } else {
            const int2 i_coord = (int2)(coord.x/2, (coord.y - dim.y/2)/2);
            out = read_imagef(upsize_light, sampler, i_coord) / fac;
        }
    } else {
        if (coord.y < dim.y/2) {
            
            const int2 i_coord = (int2)((coord.x-dim.x/2)/2, (coord.y)/2);
            //out = read_imagef(scale_dark, sampler, i_coord);
        } else {
            const int2 i_coord = (int2)((coord.x-dim.x/2)/2, (coord.y - dim.y/2)/2);
            out = read_imagef(scale_light, sampler, i_coord) / fac;
        }
    }
    
    write_imagef(output, coord, out);
}

__kernel void pad_top_bottom(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    float4 out = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
    
    if (coord.y == 0) {
        out = read_imagef(input, sampler, coord + (int2)(0, 3));
    } else if (coord.y == dim.y - 1) {
        out = read_imagef(input, sampler, coord + (int2)(0, -4));
    } else {
        out = read_imagef(input, sampler, coord + (int2)(0, -1));
    }
    
    write_imagef(output, coord, out);
}

__kernel void clip_top_bottom(read_only image2d_t input, write_only image2d_t output) {
    const sampler_t sampler = CLK_FILTER_NEAREST | CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE;
    
    const int2 coord = (int2)(get_global_id(0), get_global_id(1));
    const int2 dim = get_image_dim(output);
    
    const float4 out = read_imagef(input, sampler, coord + (int2)(0, 1));
    
    write_imagef(output, coord, out);
}

#endif


