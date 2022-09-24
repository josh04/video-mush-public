#version 400

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vert;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 texcoord;

out vec3 vPosition;

void main()
{
    vPosition = vert;
}