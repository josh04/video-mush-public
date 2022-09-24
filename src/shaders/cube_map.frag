#version 400

// Interpolated values from the vertex shaders
in vec3 vVert;

uniform samplerCube cube_map;

// Ouput data
out vec3 color;

void main() {
	float coord_x = 0.0;
	float coord_y = 0.0;
	float coord_z = 0.0;
	if (vVert.y < 0.25 && vVert.y > -0.25) {
		
		coord_y = 1.0 + 2.0 * -(vVert.y + 0.25) / 0.5;

		if (vVert.x > 0.5) {
			coord_z = (vVert.x - 0.5) * 4.0 - 1.0;
			coord_x = -1.0;
		} else if (vVert.x > 0.0) {
			coord_x = 1.0 - (vVert.x)  * 4.0;
			coord_z = -1.0;
		} else if (vVert.x > -0.5) {
			coord_z = 1.0 - (vVert.x + 0.5) * 4.0;
			coord_x = 1.0;
		} else {
			coord_x = (vVert.x + 1.0) * 4.0 - 1.0;
			coord_z = 1.0;
		}
		
		color = vec3(texture(cube_map, vec3(coord_x, coord_y, coord_z)));
	
	} else if (vVert.y > 0.25 && vVert.y < 0.75) {
		coord_y = 1.0;
		coord_z = -1.0 + 2.0 * (vVert.y - 0.25) / 0.5;
		if (vVert.x > 0.0 && vVert.x < 0.5) {
			coord_x = (vVert.x) * 4.0 - 1.0;
			
			color = vec3(texture(cube_map, vec3(coord_x, coord_y, coord_z)));
		} else {
			color = vec3(0.0);
		}
	} else if (vVert.y < -0.25 && vVert.y > -0.75) {
		coord_y = -1.0;
		
		coord_z = -1.0 + 2.0 * -(vVert.y + 0.25) / 0.5;
		
		if (vVert.x > 0.0 && vVert.x < 0.5) {
			coord_x = (vVert.x) * 4.0 - 1.0;
			
			color = vec3(texture(cube_map, vec3(coord_x, coord_y, coord_z)));
		} else {
			color = vec3(0.0);
		}
    } else {
        color = vec3(0.0);
    }
	
    
}
