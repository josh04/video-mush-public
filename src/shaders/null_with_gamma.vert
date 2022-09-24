#version 150

in vec3 vert;
in vec2 tex;
out vec2 fragTexCoord;

void main() {
    fragTexCoord = tex;
    
    gl_Position = vec4(vert, 1.0);
    
}
