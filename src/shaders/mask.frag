#version 150

vec3 processOutput(vec4 rgba, int columnParity);

uniform sampler2D tex; //this is the texture
in vec2 fragTexCoord; //this is the texture coord
out vec4 finalColor; //this is the output color of the pixel

uniform float iso;

void main() {

	finalColor = texture(tex, fragTexCoord);

//	finalColor = finalColor / vec4(iso);
	
//	finalColor = clamp(vec4(processOutput(finalColor, int(fragTexCoord.x * 1920) % 2), 1.0), 0.0f, 1.0f);

//	finalColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);

}

