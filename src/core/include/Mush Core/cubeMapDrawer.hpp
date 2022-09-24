#ifndef CUBE_MAP_DRAWER_HPP
#define CUBE_MAP_DRAWER_HPP

#include <azure/sprite.hpp>

#include "cubeMapDrawer.hpp"

namespace mush {
	namespace raster {
		class cubeMapDrawer : public azure::Sprite {
		public:
			cubeMapDrawer(std::shared_ptr<azure::Texture> cube_map) : azure::Sprite(nullptr, cube_map) {

				_program = std::make_shared<azure::Program>();
				auto shaderFactory = azure::ShaderFactory::GetInstance();
				_program->addShader(shaderFactory->get("shaders/cube_map_equirectangular.frag", GL_FRAGMENT_SHADER));
				_program->addShader(shaderFactory->get("shaders/cube_map.vert", GL_VERTEX_SHADER));
				_program->link();
				_program->use();

				_program->uniform("cube_map", (GLint)0);

				_program->discard();

				_mode = GL_TRIANGLES;
				_layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_FALSE));

				_count = 6;

				_data = {

					-1.0f,	-1.0f,	0.0f,	
					-1.0f,	1.0f,	0.0f,	
					1.0f,	1.0f,	0.0f,	

					1.0f,	1.0f,	0.0f,	
					1.0f,	-1.0f,	0.0f,	
					-1.0f,	-1.0f,	0.0f,	
				};

				buffer();
				attach();
			}

			~cubeMapDrawer() {

			}

		private:

		};
	}
}


#endif


