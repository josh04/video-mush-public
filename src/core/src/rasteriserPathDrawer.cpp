//
//  rasteriserPathDrawer.cpp
//  mush-core
//
//  Created by Josh McNamee on 15/04/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#include "rasteriserPathDrawer.hpp"
#include <azure/program.hpp>

namespace mush {
    namespace raster {
        path_drawer::path_drawer() : azure::GLObject(nullptr) {

			_program = std::make_shared<azure::Program>();
			auto shaderFactory = azure::ShaderFactory::GetInstance();
			_program->addShader(shaderFactory->get("shaders/path.frag", GL_FRAGMENT_SHADER));
			_program->addShader(shaderFactory->get("shaders/path.geom", GL_GEOMETRY_SHADER));
			_program->addShader(shaderFactory->get("shaders/path.vert", GL_VERTEX_SHADER));
			_program->link();
			_program->use();
			_program->discard();

            _mode = GL_LINES;
            _layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_FALSE));
            _layout.push_back(azure::GLObject::Layout("tick", 1, GL_FLOAT, GL_FALSE));
			_layout.push_back(azure::GLObject::Layout("pre", 3, GL_FLOAT, GL_FALSE));
			_layout.push_back(azure::GLObject::Layout("post", 3, GL_FLOAT, GL_FALSE));
            
            _count = 0;
            
            
        }
        
        path_drawer::~path_drawer() {
            
        }
        
        void path_drawer::init(const std::vector<camera::camera_path_node>& path) {
            
            _data.clear();
            _data.reserve(4 * (path.size() -1) * 2);
            _count = (path.size() - 1) * 2;
            
			auto data = { path[0].camera.location.x, path[0].camera.location.y, path[0].camera.location.z,
				(float)path[0].node_tick,
				path[0].camera.location.x, path[0].camera.location.y, path[0].camera.location.z,
				path[1].camera.location.x, path[1].camera.location.y, path[1].camera.location.z,
			};

			auto data2 = { path[1].camera.location.x, path[1].camera.location.y, path[1].camera.location.z,
				(float)path[1].node_tick,
				path[1].camera.location.x, path[1].camera.location.y, path[1].camera.location.z,
				path[2].camera.location.x, path[2].camera.location.y, path[2].camera.location.z,
			}; 

			_data.insert(_data.end(), data.begin(), data.end());
			_data.insert(_data.end(), data2.begin(), data2.end());
            
            for (int i = 1; i < path.size()-2; i++) {
                    auto data = { path[i].camera.location.x, path[i].camera.location.y, path[i].camera.location.z, 
						(float)path[i].node_tick,
						path[i-1].camera.location.x, path[i-1].camera.location.y, path[i-1].camera.location.z,
						path[i+1].camera.location.x, path[i+1].camera.location.y, path[i+1].camera.location.z,
					};
                    _data.insert(_data.end(), data.begin(), data.end());
                
					auto data2 = { path[i + 1].camera.location.x, path[i + 1].camera.location.y, path[i + 1].camera.location.z,
						(float)path[i + 1].node_tick,
						path[i].camera.location.x, path[i].camera.location.y, path[i].camera.location.z,
						path[i + 2].camera.location.x, path[i + 2].camera.location.y, path[i + 2].camera.location.z,
					}; 
					_data.insert(_data.end(), data2.begin(), data2.end());
            }

			auto p = path.size() - 2;
			auto p2 = path.size() - 1;

			auto data3 = { path[p].camera.location.x, path[p].camera.location.y, path[p].camera.location.z,
				(float)path[p].node_tick,
				path[p-1].camera.location.x, path[p-1].camera.location.y, path[p-1].camera.location.z,
				path[p+1].camera.location.x, path[p+1].camera.location.y, path[p+1].camera.location.z,
			};

			auto data4 = { path[p+1].camera.location.x, path[p+1].camera.location.y, path[p+1].camera.location.z,
				(float)path[p+1].node_tick,
				path[p].camera.location.x, path[p].camera.location.y, path[p].camera.location.z,
				path[p+1].camera.location.x, path[p+1].camera.location.y, path[p+1].camera.location.z,
			};

			_data.insert(_data.end(), data3.begin(), data3.end());
			_data.insert(_data.end(), data4.begin(), data4.end());

            buffer();
        }
        
        void path_drawer::render() {
            glEnable(GL_PROGRAM_POINT_SIZE);
            GLObject::render();
            glDisable(GL_PROGRAM_POINT_SIZE);
        }
    }
}
