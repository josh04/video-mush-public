#version 150

// --------------------------------------------------------------------------
// Sim2 Shader v3.0
// Converted to GLSL 1.50 by Tim Law.
// --------------------------------------------------------------------------

const float RGB2XYZ_XR = 0.412137;
const float RGB2XYZ_XG = 0.376845;
const float RGB2XYZ_XB = 0.142235;
const float RGB2XYZ_YR = 0.221852;
const float RGB2XYZ_YG = 0.757303;
const float RGB2XYZ_YB = 0.081677;
const float RGB2XYZ_ZR = 0.011588;
const float RGB2XYZ_ZG = 0.069829;
const float RGB2XYZ_ZB = 0.706036;
const float LUMINANCE_BOOST = 1.0000;

vec3 RGBtoXYZ(vec3 RGB) {
    float X = RGB2XYZ_XR * RGB.r + RGB2XYZ_XG * RGB.g + RGB2XYZ_XB * RGB.b;
    float Y = RGB2XYZ_YR * RGB.r + RGB2XYZ_YG * RGB.g + RGB2XYZ_YB * RGB.b;
    float Z = RGB2XYZ_ZR * RGB.r + RGB2XYZ_ZG * RGB.g + RGB2XYZ_ZB * RGB.b;
    return vec3(X, Y, Z);
}

float XYZtou(float X, float Y, float Z) {
    return (((1626.6875 * X) / (X + 15.0 * Y + 3.0 * Z)) + 0.546875) * 4.0;
}

float XYZtov(float X, float Y, float Z) {
    return (((3660.046875 * Y) / (X + 15.0 * Y + 3.0 * Z)) + 0.546875) * 4.0;
}

vec3 RGBtoXYZforuv(vec3 RGB) {
    return RGBtoXYZ(RGB);
}

float RGBtou(vec3 RGB) {
    vec3 XYZ = RGBtoXYZforuv(RGB);
    return XYZtou(XYZ.r, XYZ.g, XYZ.b);
}

float RGBtov(vec3 RGB) {
    vec3 XYZ = RGBtoXYZforuv(RGB);
    return XYZtov( XYZ.r, XYZ.g, XYZ.b);
}

vec3 processOutput(vec3 rgb, int columnParity, vec3 texleft, vec3 texright, float num) {
    rgb = pow(rgb, vec3(2.2f));
    texleft = pow(texleft, vec3(2.2f));
    texright = pow(texright, vec3(2.2f));
    
    //Scale RGB up to 0-255
    rgb *= 255.0;
	texleft *= 255.0;
	texright *= 255.0;
	
    //RGB to XYZ conversion.
    float sc = 1.0 * LUMINANCE_BOOST;
    vec3 XYZ = RGBtoXYZ(rgb * sc);
    float Y = XYZ.g;
    
    //Calculate u and v values.
    float u = RGBtou(rgb) * 1;
    float v = RGBtov(rgb) * 1;
    
	texleft = texleft * sc;
	u += RGBtou(texleft) * 1;
	v += RGBtov(texleft) * 1;

    texright = texright * sc;
	u += RGBtou(texright) * 1;
	v += RGBtov(texright) * 1;

	u /= num;
	v /= num;

    //Encode fragment as logLuv.
    float alpha = 0.0376;
    float L;
    if (Y < 0.00001) {
        L = 0.0;
    } else {
        L = (alpha * log2(Y)) + 0.5;
    }

    float Lscale = 32;
    L = (253.0 * L + 1.0) * Lscale;
    L = floor(L + 0.5);
    L = clamp(L, 32.0, 8159.0);        
    float Lh = floor(L / 32.0);           
    float Ll = L - (Lh * 32.0);   
        
    float Ch, Cl;
    vec3 logLuv;
    if (columnParity == 0) {
        v = floor(v + 0.5);
        v = clamp(v, 4.0, 1019.0);     
        Ch = floor(v / 4.0);        
        Cl = v - (Ch * 4.0);        

        logLuv.r = (Ll * 8.0) + (Cl * 2.0);
        logLuv.r = clamp(logLuv.r, 1.0, 254.0);   
        logLuv.g = Lh;
        logLuv.b = Ch;
    } else {
        u = floor(u + 0.5);
        u = clamp(u, 4.0, 1019.0);     
        Ch = floor(u / 4.0);        
        Cl = u - (Ch * 4.0);        

        logLuv.r = Ch;
        logLuv.g = Lh;
        logLuv.b = (Ll * 8.0) + (Cl * 2.0);
        logLuv.b = clamp(logLuv.b, 1.0, 254.0);
    }

    logLuv /= 255.0;
    return logLuv;
}

