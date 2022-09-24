#version 400

// Interpolated values from the vertex shaders
in vec3 gPosition_worldspace;
in vec3 gNormal_cameraspace;
in vec3 gEyeDirection_cameraspace;
in vec3 gLightDirection_cameraspace;
//in vec3 gFacetNormal;
in vec3 gPatchDistance;
in vec3 gTriDistance;

// Ouput data
out vec3 color;

// Values that stay constant for the whole mesh.
//uniform mat4 MV;
uniform vec3 light_position;

void main() {
    vec3 surfaceToLight = light_position - gPosition_worldspace;
    
    color = vec3(min(length(surfaceToLight), 10000.0f));
}
