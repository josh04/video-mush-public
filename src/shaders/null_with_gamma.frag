#version 150

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D tex;
uniform float gamma;

void main() {
    finalColor = pow(vec4(texture(tex, fragTexCoord)), vec4(gamma));
}
