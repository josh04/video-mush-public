//
//  sphereGL.cpp
//  video-mush
//
//  Created by Josh McNamee on 07/12/2016.
//
//

#define _USE_MATH_DEFINES
#include <math.h>

#include "sphereGL2.hpp"
#include "indexGLObject.hpp"
#include "camera_event_handler.hpp"

#include <azure/Texture.hpp>

#include <azure/glm/gtc/matrix_inverse.hpp>
#include <azure/glm/gtc/type_ptr.hpp>
#include <azure/glm/gtx/string_cast.hpp>

namespace mush {
    namespace raster {

        sphere::sphere(glm::vec3 location, float radius, unsigned int tex_width, unsigned int tex_height) : azure::indexGLObject(nullptr), _width(tex_width), _height(tex_height) {
            
            const char * vert = "shaders/sphere.vert";
            const char * frag = "shaders/sphere.frag";
            
            _program = std::make_shared<azure::Program>(vert, frag);
            _program->use();
            _program->uniform("gammma", 1.0f);
            _program->uniform("tex", 0);
            _program->discard();
            
            _texture = std::make_shared<azure::Texture>();
            _texture->createTexture(GL_RGBA32F, _width, _height, GL_RGBA, GL_FLOAT, NULL);
            
            _texture->setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            _texture->setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            //_texture->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            _texture->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            //_texture->setParam(GL_GENERATE_MIPMAP, GL_TRUE);
            _texture->setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            _texture->applyParams();
            
            
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
                    *v++ = s*S;
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

        sphere::~sphere() {
            
        }

        void sphere::render() {
            _texture->bind();
            
            azure::indexGLObject::render();
            
            _texture->unbind();
        }

    }
}
