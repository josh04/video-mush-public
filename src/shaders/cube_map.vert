#version 400

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vert;

out vec3 vVert;

void main()
{
    vVert = vert;
	gl_Position = vec4(vert, 1.0);
}