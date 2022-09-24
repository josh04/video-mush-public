

#include "oculusEyeSprite.hpp"

namespace mush {
	oculusEyeSprite::oculusEyeSprite(std::shared_ptr<azure::Program> program, std::shared_ptr<azure::Texture> texture) : azure::Sprite(program, texture) {

		_mode = GL_TRIANGLES;

		_layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_TRUE));
		_layout.push_back(azure::GLObject::Layout("tex", 2, GL_FLOAT, GL_FALSE));

		_first = 0;
		_count = 6;

		GLfloat temp[] = {
		   -1.0f,    -1.0f,	  0.0f,	0.0f, 1.0f,
		   -1.0f,     1.0f,   0.0f,	0.0f, 0.0f,
			1.0f,     1.0f,   0.0f,	1.0f, 0.0f,

			1.0f,     1.0f,   0.0f,	1.0f, 0.0f,
			1.0f,    -1.0f,	  0.0f,	1.0f, 1.0f,
		   -1.0f,    -1.0f,	  0.0f,	0.0f, 1.0f
		};

		_data.assign(temp, temp + 30);
		buffer();
		attach();
	}

	oculusEyeSprite::~oculusEyeSprite() {

	}



}