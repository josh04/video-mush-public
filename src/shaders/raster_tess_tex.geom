#version 400

// Geometry

uniform mat3 NormalMatrix;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 tePosition_worldspace[3];
in vec3 teNormal_cameraspace[3];
in vec3 teEyeDirection_cameraspace[3];
in vec3 teLightDirection_cameraspace[3];
in vec3 tePatchDistance[3];
in vec2 teTex[3];

out vec3 gPosition_worldspace;
out vec3 gNormal_cameraspace;
out vec3 gEyeDirection_cameraspace;
out vec3 gLightDirection_cameraspace;
//out vec3 gFacetNormal;
out vec3 gPatchDistance;
out vec3 gTriDistance;
out vec2 gTex;


void main()
{
/*
    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
    gFacetNormal = NormalMatrix * normalize(cross(A, B));
  */  
	gPosition_worldspace = tePosition_worldspace[0];
	gNormal_cameraspace = teNormal_cameraspace[0];
	gEyeDirection_cameraspace = teEyeDirection_cameraspace[0];
	gLightDirection_cameraspace = teLightDirection_cameraspace[0];
    gPatchDistance = tePatchDistance[0];
    gTriDistance = vec3(1, 0, 0);
    gTex = teTex[0];
    gl_Position = gl_in[0].gl_Position; EmitVertex();
	
	gPosition_worldspace = tePosition_worldspace[1];
	gNormal_cameraspace = teNormal_cameraspace[1];
	gEyeDirection_cameraspace = teEyeDirection_cameraspace[1];
	gLightDirection_cameraspace = teLightDirection_cameraspace[1];
    gPatchDistance = tePatchDistance[1];
    gTriDistance = vec3(0, 1, 0);
    gTex = teTex[1];
    gl_Position = gl_in[1].gl_Position; EmitVertex();
	
	gPosition_worldspace = tePosition_worldspace[2];
	gNormal_cameraspace = teNormal_cameraspace[2];
	gEyeDirection_cameraspace = teEyeDirection_cameraspace[2];
	gLightDirection_cameraspace = teLightDirection_cameraspace[2];
    gPatchDistance = tePatchDistance[2];
    gTriDistance = vec3(0, 0, 1);
    gTex = teTex[2];
    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}
