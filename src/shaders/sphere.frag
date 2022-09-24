#version 330

// Interpolated values from the vertex shaders
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;
in vec2 fragTexCoord;

// Ouput data
out vec4 color;

// Values that stay constant for the whole mesh.
//uniform mat4 MV;
uniform vec3 light_position;

uniform sampler2D tex;

uniform float gamma;

void main(){
    /*
     // Light emission properties
     // You probably want to put them as uniforms
     vec3 LightColor = vec3(1,1,1);
     float LightPower = 50.0f;
     
     // Material properties
     vec3 MaterialDiffuseColor = vec3(1, 1, 1);
     vec3 MaterialAmbientColor = vec3(0.6,0.6,0.6) * MaterialDiffuseColor;
     vec3 MaterialSpecularColor = vec3(0.3,0.3,0.3);
     
     // Distance to the light
     float distance = length( light_position - Position_worldspace );
     
     // Normal of the computed fragment, in camera space
     vec3 n = normalize( Normal_cameraspace );
     // Direction of the light (from the fragment to the light)
     vec3 l = normalize( LightDirection_cameraspace );
     // Cosine of the angle between the normal and the light direction,
     // clamped above 0
     //  - light is at the vertical of the triangle -> 1
     //  - light is perpendicular to the triangle -> 0
     //  - light is behind the triangle -> 0
     float cosTheta = clamp( dot( n,l ), 0,1 );
     
     // Eye vector (towards the camera)
     vec3 E = normalize(EyeDirection_cameraspace);
     // Direction in which the triangle reflects the light
     vec3 R = reflect(-l,n);
     // Cosine of the angle between the Eye vector and the Reflect vector,
     // clamped to 0
     //  - Looking into the reflection -> 1
     //  - Looking elsewhere -> < 1
     float cosAlpha = clamp( dot( E,R ), 0,1 );
     
     color =
     // Ambient : simulates indirect lighting
     MaterialAmbientColor +
     // Diffuse : "color" of the object
     MaterialDiffuseColor * LightColor * LightPower * cosTheta / (distance*distance) +
     // Specular : reflective highlight, like a mirror
     MaterialSpecularColor * LightColor * LightPower * pow(cosAlpha,5) / (distance*distance);
     */
    
    //color = color / (1.0 + color);
    
    vec3 surfaceToLight = light_position - Position_worldspace;
    
    // Normal of the computed fragment, in camera space
    vec3 n = normalize( Normal_cameraspace );
    // Direction of the light (from the fragment to the light)
    vec3 l = normalize( LightDirection_cameraspace );
    
    float brightness = dot(-n, l) / (length(l) * length(n));
    //brightness = clamp(abs(brightness), 0, 1);

    //color = pow(vec4(vec3(brightness) * texture(tex, fragTexCoord).xyz, 1.0f),  vec4(gamma));
    color = vec4(texture(tex, fragTexCoord).xyz, 1.0f);
    //gl_FragDepth = 1.0f;
	
}