#version 400

// Interpolated values from the vertex shaders
in vec3 vVert;

uniform samplerCube cube_map;

// Ouput data
out vec3 color;


#define M_PI 3.1415926535897932384626433832795

mat4 rotationMatrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

vec3 rotate(vec3 v, vec3 axis, float angle) {
    mat4 m = rotationMatrix(axis, angle);
    return (m * vec4(v, 1.0)).xyz;
}

void main() {
    
    float x_ang = - M_PI * 0.5 + 2.0 * M_PI * (vVert.x + 1.0) / 2.0;
    float y_ang = M_PI * (vVert.y + 1.0) / 2.0;
    
    vec3 up = vec3(0,1,0);
    
    vec3 target = rotate(rotate(up, vec3(0,0,1), y_ang), vec3(0,1,0), x_ang);
    
    color = vec3(texture(cube_map, target));
        
    
    
}
