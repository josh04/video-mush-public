#version 330

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vert;
layout(location = 1) in float tick;

layout(location = 2) in vec3 pre;
layout(location = 3) in vec3 post;

out float tick_f;

flat out vec3 pre_g;
flat out vec3 post_g;




void main(){
    
    gl_Position =  vec4(vert,1);
    gl_PointSize = 3.0f;
    tick_f = tick;
    
	pre_g = pre;
	post_g = post;
}
