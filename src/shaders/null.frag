#version 150

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D tex;

void main() {
    finalColor = vec4(texture(tex, fragTexCoord));
}
