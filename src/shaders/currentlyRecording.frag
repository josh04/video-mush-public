//
//  currentlyRecording.frag
//  video-mush
//
//  Created by Josh McNamee on 15/10/2014.
//
//

#version 150

vec3 processOutput(vec3 rgb, int columnParity, vec3 texleft, vec3 texright, float num);

uniform sampler2D tex; //this is the texture
in vec2 fragTexCoord; //this is the texture coord
out vec4 finalColor; //this is the output color of the pixel

uniform float exposure; //this is the exposure
uniform int width; //this is the wodth
uniform int height; //this is the wodth

void main() {
    finalColor = texture(tex, fragTexCoord) * exposure;
    vec4 texleft = texture(tex, fragTexCoord + vec2((1/width), 0.0)) * exposure;
    vec4 texright = texture(tex, fragTexCoord + vec2((1/width), 0.0)) * exposure;
    
    float num = 3.0;
    
    if (fragTexCoord.x == 0.0) {
        num = 2.0;
        texleft = vec4(0.0);
    } else if (fragTexCoord.x == 1.0) {
        num = 2.0;
        texright = vec4(0.0);
    }
    
    const float pix_size = 10.0;
    
    float mix = min(max(pix_size*4.0 - min(fragTexCoord.x*width, pix_size) - min((1.0 - fragTexCoord.x)*width, pix_size) - min(fragTexCoord.y*height, pix_size) - min((1.0 - fragTexCoord.y)*height, pix_size), 0.0) / pix_size, 1.0)/2.0;
    
    finalColor = (finalColor * finalColor.w * (1.0 - mix) + vec4(1.0, 0.0, 0.0, mix) * mix) / (mix + finalColor.w * (1-mix));
    
    /*
    if (fragTexCoord.x*width < 50.0 || (1.0 - fragTexCoord.x)*width < 50.0 || fragTexCoord.y*height < 50.0 || (1.0 - fragTexCoord.y)*height < 50.0) {
        
        finalColor = vec4(1.0, 0.0, 0.0, 1.0);
        texleft = vec4(1.0, 0.0, 0.0, 1.0);
        texright = vec4(1.0, 0.0, 0.0, 1.0);
    }*/
    
    
    finalColor = clamp(vec4(processOutput(finalColor.rgb, int(fragTexCoord.x * width) % 2, texleft.rgb, texright.rgb, num), 1.0), 0.0f, 1.0f);
    
}
