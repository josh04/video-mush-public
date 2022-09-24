//
//  sphereGL.cpp
//  video-mush
//
//  Created by Josh McNamee on 07/12/2016.
//
//

#define _USE_MATH_DEFINES
#include <math.h>

#include "sphereGL.hpp"
#include "indexGLObject.hpp"
#include "camera_event_handler.hpp"


#include <azure/glm/gtc/matrix_inverse.hpp>
#include <azure/glm/gtc/type_ptr.hpp>
#include <azure/glm/gtx/string_cast.hpp>

sphereGL::sphereGL(std::shared_ptr<azure::Program> program, glm::vec3 location, float radius) : azure::indexGLObject(program) {
    
    _layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_FALSE));
    _layout.push_back(azure::GLObject::Layout("norm", 3, GL_FLOAT, GL_TRUE));
    _layout.push_back(azure::GLObject::Layout("texcoord", 2, GL_FLOAT, GL_FALSE));
    
    int rings = 48;
    int sectors = 96;
    float const R = 1.0f / (float)(rings-1);
    float const S = 1.0f / (float)(sectors-1);
    
    _data.resize(rings * sectors * (3 + 3 + 2));
    
    std::vector<GLfloat>::iterator v = _data.begin();
    
    _count = 0;
    for(int r = 0; r < rings; r++) {
		for (int s = 0; s < sectors; s++) {
			float const y = sin(-M_PI_2 + M_PI * r * R);
			float const x = cos(2 * M_PI * s * S) * sin(M_PI * r * R);
			float const z = sin(2 * M_PI * s * S) * sin(M_PI * r * R);


			*v++ = x * radius + location.x;
			*v++ = y * radius + location.y;
			*v++ = z * radius + location.z;

			*v++ = x;
			*v++ = y;
			*v++ = z;
//#ifdef _WIN32
			//*v++ = s*S;
//#endif
//#ifdef __APPLE__
			*v++ = 1.0f - s*S;
//#endif
			*v++ = r*R;

			_count++;
		}
    }
    
    indices.resize(rings * sectors * 6);
    
    std::vector<unsigned int>::iterator i = indices.begin();
    
    for(int r = 0; r < rings-1; r++) {
        for(int s = 0; s < sectors-1; s++) {
            *i++ = r * sectors + s;
            *i++ = r * sectors + (s+1);
            *i++ = (r+1) * sectors + (s+1);
            *i++ = (r+1) * sectors + (s+1);
            *i++ = (r+1) * sectors + s;
            *i++ = r * sectors + s;
        }
    }
    
    index_buffer();
    buffer();
    index_attach();
    //attach();
    
}

sphereGL::~sphereGL() {
    
}
