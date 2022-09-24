#version 150

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D tex;

uniform float exposure;

uniform int selector;

vec3 tfdecode(vec3 rgb);

vec4 bt709rgbyuv(const vec4 i) {
     float y = i.x*0.2126 +    i.y*0.7152 +    i.z*0.0722;
     float u = i.x*-0.1146 +   i.y*-0.3854 +   i.z*0.5;
     float v = i.x*0.5 +       i.y*-0.4542 +   i.z*-0.0468;
    return vec4(y, u+0.5f, v+0.5f, 1.0f);
}

vec4 bt709yuvrgb(vec4 i) {
    i.y = i.y - 0.5f;
    i.z = i.z - 0.5f;
     float r = i.x*1 + i.y*0 +         i.z*1.570;
     float g = i.x*1 + i.y*-0.187 +    i.z*-0.467;
     float b = i.x*1 + i.y*1.856 +     i.z*0;
    return vec4(r, g, b, 1.0f);
}

vec4 bt709_luminance(vec4 color) {
    vec4 temp = bt709rgbyuv(color);
    return vec4(temp.x, temp.x, temp.x, color.w);
}

vec4 bt709_u(vec4 color) {
    vec4 temp = bt709rgbyuv(color);
    temp.x = 0.5f;
    temp.z = 0.5f;
    vec4 temp2 = bt709yuvrgb(temp);
    return vec4(temp2.x, temp2.y, temp2.z, color.w);
}

vec4 bt709_v(vec4 color) {
    vec4 temp = bt709rgbyuv(color);
    temp.x = 0.5f;
    temp.y = 0.5f;
    vec4 temp2 = bt709yuvrgb(temp);
    return vec4(temp2.x, temp2.y, temp2.z, color.w);
}

vec4 bt709_r(vec4 color) {
    return vec4(color.x, 0.0, 0.0, color.w);
}

vec4 bt709_g(vec4 color) {
    return vec4(0.0, color.y, 0.0, color.w);
}

vec4 bt709_b(vec4 color) {
    return vec4(0.0, 0.0, color.z, color.w);
}

vec4 bt709_rrr(vec4 color) {
    return vec4(color.x, color.x, color.x, color.w);
}

vec4 bt709_ggg(vec4 color) {
    return vec4(color.y, color.y, color.y, color.w);
}

vec4 bt709_bbb(vec4 color) {
    return vec4(color.z, color.z, color.z, color.w);
}

void main() {
    vec4 color = vec4(tfdecode(texture(tex, fragTexCoord).rgb) * vec3(exposure), 1.0);
    
    switch(selector) {
        default:
		case 13:
        case 0:
            finalColor = color;
            break;
        case 1:
            finalColor = bt709_luminance(color);
            break;
        case 2:
            finalColor = bt709_u(color);
            break;
        case 3:
            finalColor = bt709_v(color);
            break;
        case 4:
            finalColor = bt709_r(color);
            break;
        case 5:
            finalColor = bt709_g(color);
            break;
        case 6:
            finalColor = bt709_b(color);
            break;
        case 7:
            finalColor = bt709_rrr(color);
            break;
        case 8:
            finalColor = bt709_ggg(color);
            break;
        case 9:
            finalColor = bt709_bbb(color);
            break;
        case 10:
		{
			vec4 l_color = vec4(tfdecode(texture(tex, vec2(fragTexCoord.x / 2.0, fragTexCoord.y)).rgb) * vec3(exposure), 1.0);
			vec4 r_color = vec4(tfdecode(texture(tex, vec2(0.5 + fragTexCoord.x / 2.0, fragTexCoord.y)).rgb) * vec3(exposure), 1.0);
            finalColor = vec4(l_color.x, r_color.y, r_color.z, r_color.w);
            break;
			}
        case 11:
		{
			vec4 l_color = vec4(tfdecode(texture(tex, vec2(fragTexCoord.x / 2.0, fragTexCoord.y)).rgb) * vec3(exposure), 1.0);
            finalColor = l_color;
            break;
			}
        case 12:
		{
			vec4 r_color = vec4(tfdecode(texture(tex, vec2(0.5 + fragTexCoord.x / 2.0, fragTexCoord.y)).rgb) * vec3(exposure), 1.0);
            finalColor = r_color;
            break;
			}
    }
}
