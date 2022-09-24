//
//  decodeUDP.cl
//  video-mush
//
//  Created by Visualisation on 20/03/2014.
//  Copyright (c) 2014. All rights reserved.
//

/*__kernel void decodeUDP(write_only image2d_t output, __global uchar * input) {
	int x = get_global_id(0), y = get_global_id(1);
	float luma = 1.0;
	int i = ((1280)*y + x*2);
	float y1 = (((((short)(input[4*i+1]&0x00ff)) ) * 256 + ((short)(input[4*i+0])&0x00ff)))*luma;  //-0x0039)*(0xffff/(0x3ac-0x0040));
	float y2 = (((((short)(input[4*i+5]&0x00ff)) ) * 256 + ((short)(input[4*i+4])&0x00ff)))*luma;  //-0x0039)*(0xffff/(0x3ac-0x0040));
	
	if(y1 > 65535) {
		y1=65535;
	}
	
	if(y2 > 65535) {
		y2=65535;
	}
	/*
	short v =  ((((short)(line[4*i+1]&0xfc)))/4  +((short)(line[4*i+2]&0x0f)*64) - 511)*16 ;
	short u =  ((((short)(line[4*i+5]&0xfc)))/4  +((short)(line[4*i+6]&0x0f)*64) - 511)*16 ;
	//printf("      %x %x %d %d\n",y1,y2,v,u);
	image2.at<uint16_t>(3*i)   = 	(uint16_t)y1;// + v*1.28033;
	image2.at<uint16_t>(3*i+1) = 	(uint16_t)y1;// - u*0.21482- v*0.38059;
	image2.at<uint16_t>(3*i+2) = 	(uint16_t)y1;// + u*2.12798;
	image2.at<uint16_t>(3*i+3) = 	(uint16_t)y2;// + v*1.28033;
	image2.at<uint16_t>(3*i+4) = 	(uint16_t)y2;// - u*0.21482- v*0.38059;
	image2.at<uint16_t>(3*i+5) = 	(uint16_t)y2;// + u*2.12798;*
	
	write_imagef(output, (int2)((x*2), y), (float4)(y1, y1, y1, 1.0));
	write_imagef(output, (int2)((x*2)+1, y), (float4)(y2, y2, y2, 1.0));
	
}*/

__kernel void decodeUDP(write_only image2d_t output, __global uchar * input) {
	int x = get_global_id(0), y = get_global_id(1);
	float luma = 1.0, sat = 0.8;
	int ep = (640*y) + x;
	
	float y1 = ((short)(input[4*ep+0])&0x00ff);//-0x0039)*(0xffff/(0x3ac-0x0040));
	float y2 = ((short)(input[4*ep+1])&0x00ff);//-0x0039)*(0xffff/(0x3ac-0x0040));
	float u =  ((short)(input[4*ep+2]&0x007f) - 64)*2;
	float v =  (((short)(input[4*ep+3]&0x003f))*2 + ((short)(input[4*ep+2]&0x0080))/128 - 64)*2;
	int shift = ((short)(input[4*ep+3])&0x00c0)/64;
	
	if(shift == 0){
		y1 = y1*256;
		y2 = y2*256;
		u  = u*256;
		v  = v*256;
	}
	if(shift == 1){
		y1 = y1*64;
		y2 = y2*64;
		u  = u*64;
		v  = v*64;
	}
	if(shift == 2){
		y1 = y1*8;
		y2 = y2*8;
		u  = u*8;
		v  = v*8;
	}
	y1 = y1 * luma;
	y2 = y2 * luma;
	u = u * luma*sat;
	v = v * luma*sat;
	
	float R1 = y2 + v*1.28033;
	float G1 = y2 - u*0.21482- v*0.38059;
	float B1 = y2 + u*2.12798;
	float R2 = y1 + v*1.28033;
	float G2 = y1 - u*0.21482- v*0.38059;
	float B2 = y1 + u*2.12798;
	
	if(R1 > 65535)R1 = 65535;
	if(R1 < 0)R1 = 0;
	if(G1 > 65535)G1 = 65535;
	if(G1 < 0)G1 = 0;
	if(B1 > 65535)B1 = 65535;
	if(B1 < 0)B1 = 0;
	
	if(R2 > 65535)R2 = 65535;
	if(R2 < 0)R2 = 0;
	if(G2 > 65535)G2 = 65535;
	if(G2 < 0)G2 = 0;
	if(B2 > 65535)B2 = 65535;
	if(B2 < 0)B2 = 0;
	
	
	//printf("      %f %f %f %f %d\n",y1,y2,v,u, shift);
	
	write_imagef(output, (int2)((x*2), y), (float4)(R1, G1, B1, 1.0));
	write_imagef(output, (int2)((x*2)+1, y), (float4)(R2, G2, B2, 1.0));

}

