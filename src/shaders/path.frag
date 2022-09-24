#version 330

in float tick_f2;

// Ouput data
out vec4 color;


void main(){
    
    color = vec4(tick_f2 / 256.0, 0.1f, 0.1f, 1.0f);
    
}
