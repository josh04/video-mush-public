#version 150
in vec3 vert;
in vec2 vertTexCoord;
out vec2 fragTexCoord;

uniform float scroll;

void main() {
    // Pass the tex coord straight through to the fragment shader
    fragTexCoord = vertTexCoord;
    
    gl_Position = vec4(vert+vec3(0,scroll,0), 1);
}

