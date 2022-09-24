#include <azure/program.hpp>
#include "rasteriserSceneTesselation.hpp"
#include "tinyobjloader/tiny_obj_loader.h"
#include <boost/filesystem.hpp>
#include "mushLog.hpp"

#include <azure/Texture.hpp>
#include <scarlet/freeimageplus.hpp>

#include <sstream>

//#include <assimp/Importer.hpp>

#include <math.h>
#include <array>

namespace mush {
	namespace raster {
		sceneTesselation::sceneTesselation() : azure::indexGLObject(nullptr) {

			_program = std::make_shared<azure::Program>();
			auto shaderFactory = azure::ShaderFactory::GetInstance();
			_program->addShader(shaderFactory->get("shaders/raster_tess.frag", GL_FRAGMENT_SHADER));
			_program->addShader(shaderFactory->get("shaders/raster_tess.tess_control", GL_TESS_CONTROL_SHADER));
			_program->addShader(shaderFactory->get("shaders/raster_tess.tess_eval", GL_TESS_EVALUATION_SHADER));
			_program->addShader(shaderFactory->get("shaders/raster_tess.geom", GL_GEOMETRY_SHADER));
			_program->addShader(shaderFactory->get("shaders/raster_tess.vert", GL_VERTEX_SHADER));
			_program->link();
			_program->use();
			_program->discard();
            _regular_program = _program;
            _depth_program = std::make_shared<azure::Program>();
            _depth_program->addShader(shaderFactory->get("shaders/raster_tess_depth.frag", GL_FRAGMENT_SHADER));
            _depth_program->addShader(shaderFactory->get("shaders/raster_tess.tess_control", GL_TESS_CONTROL_SHADER));
            _depth_program->addShader(shaderFactory->get("shaders/raster_tess.tess_eval", GL_TESS_EVALUATION_SHADER));
            _depth_program->addShader(shaderFactory->get("shaders/raster_tess.geom", GL_GEOMETRY_SHADER));
            _depth_program->addShader(shaderFactory->get("shaders/raster_tess.vert", GL_VERTEX_SHADER));
            _depth_program->link();
            _depth_program->use();
            _depth_program->discard();
            
            _texture_program = std::make_shared<azure::Program>();
            _texture_program->addShader(shaderFactory->get("shaders/raster_tess_tex.frag", GL_FRAGMENT_SHADER));
            _texture_program->addShader(shaderFactory->get("shaders/raster_tess_tex.tess_control", GL_TESS_CONTROL_SHADER));
            _texture_program->addShader(shaderFactory->get("shaders/raster_tess_tex.tess_eval", GL_TESS_EVALUATION_SHADER));
            _texture_program->addShader(shaderFactory->get("shaders/raster_tess_tex.geom", GL_GEOMETRY_SHADER));
            _texture_program->addShader(shaderFactory->get("shaders/raster_tess_tex.vert", GL_VERTEX_SHADER));
            _texture_program->link();
            _texture_program->use();
            _texture_program->uniform("tex", 0);
            _texture_program->discard();
            
            
		}

		sceneTesselation::sceneTesselation(const sceneTesselation & other) : azure::indexGLObject(other) {

		}

		sceneTesselation::~sceneTesselation() {

		}

		void sceneTesselation::init(const char * model_path, const float model_scale) {
			_mode = GL_PATCHES;
			_layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_FALSE));
			_layout.push_back(azure::GLObject::Layout("norm", 3, GL_FLOAT, GL_TRUE));
			_layout.push_back(azure::GLObject::Layout("texcoord", 2, GL_FLOAT, GL_FALSE));

			load_model(model_path, model_scale);
            
            std::stringstream strm;
            
            strm << "Triangle count: " << indices.size() / (3.0f);
            
            putLog(strm.str());

			index_buffer();
			buffer();
			index_attach();
			attach();
		}

		void sceneTesselation::render() {
            if (_use_depth_program) {
                _program = _depth_program;
            }
			glPatchParameteri(GL_PATCH_VERTICES, 3);
			_glException();
            
            for (auto& i : _material_groups) {
                if (!_use_depth_program) {
                    if (i.second.texture == nullptr) {
                        _program = _regular_program;
                    } else {
                        _program = _texture_program;
                        i.second.texture->bind();
                    }
                }
                
                if (i.second.length > 0) {
                    //index_buffer(i.second.first);
                    //index_attach();
                    indexGLObject::render(i.second.offset, i.second.length);
                }
                
                /*if (i.second.texture != nullptr) {
                    i.second.texture->unbind();
                }*/
            }
            
            if (_use_depth_program) {
                _program = _regular_program;
            }
		}

		std::array<float, 3> sceneTesselation::CalcNormal(float v0[3], float v1[3], float v2[3]) {
			std::array<float, 3> N;
			float v10[3];
			v10[0] = v1[0] - v0[0];
			v10[1] = v1[1] - v0[1];
			v10[2] = v1[2] - v0[2];

			float v20[3];
			v20[0] = v2[0] - v0[0];
			v20[1] = v2[1] - v0[1];
			v20[2] = v2[2] - v0[2];

			N[0] = v20[1] * v10[2] - v20[2] * v10[1];
			N[1] = v20[2] * v10[0] - v20[0] * v10[2];
			N[2] = v20[0] * v10[1] - v20[1] * v10[0];

			float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
			if (len2 > 0.0f) {
				float len = sqrtf(len2);

				N[0] /= len;
				N[1] /= len;
			}
			return N;
		}

		void sceneTesselation::load_model(const char * model_path, const float model_scale) {
			boost::filesystem::path p(model_path);

			// Loader data
			std::vector<tinyobj_mush::shape_t> shapes;
			std::vector<tinyobj_mush::material_t> materials;

			tinyobj_mush::material_t default_material;

			default_material.name = "glass";

			default_material.diffuse[0] = 0.5f;
			default_material.diffuse[1] = 0.5f;
			default_material.diffuse[2] = 0.5f;

			default_material.ior = 1.52f;

			auto d = p.remove_filename();
			d += boost::filesystem::path::preferred_separator;
			std::string error = "";

			// Try loading file
			tinyobj_mush::attrib_t attrib;

			tinyobj_mush::LoadObj(&attrib, &shapes, &materials, &error, model_path, d.generic_string().c_str());
			if (error != "")
			{
				putLog(error);
				//throw std::runtime_error(error);
			}

			if (shapes.size() == 0) {
				throw std::runtime_error("No object loaded.");
			}

			size_t index_count = 0;

			if (attrib.normals.size() == 0) {
				// Generate normals
			}

			size_t indices_count = 0;
			for (size_t s = 0; s < shapes.size(); ++s) {
				indices_count += shapes[s].mesh.indices.size();
			}

			indices.reserve(indices_count);
			_data.reserve(indices_count * 8);
            
            
            std::map<int, std::vector<unsigned int>> material_indices;

			// Loop over shapes
			for (size_t s = 0; s < shapes.size(); s++) {

				std::array<float, 3> normals;

				for (int i = 0; i < shapes[s].mesh.indices.size(); ++i) {

					_data.push_back(attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index] * model_scale);
					_data.push_back(attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index + 1] * model_scale);
					_data.push_back(attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index + 2] * model_scale);

					if (attrib.normals.size() == 0) {
						// Generate normals
						if (i % 3 == 0) {
							float * v1 = &attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index];

							float * v2 = &attrib.vertices[3 * shapes[s].mesh.indices[i + 1].vertex_index];

							float * v3 = &attrib.vertices[3 * shapes[s].mesh.indices[i + 2].vertex_index];

							normals = CalcNormal(v1, v2, v3);

						}

						_data.push_back(-normals[0]);
						_data.push_back(-normals[1]);
						_data.push_back(-normals[2]);

					} else {

						_data.push_back(attrib.normals[3 * shapes[s].mesh.indices[i].normal_index]);
						_data.push_back(attrib.normals[3 * shapes[s].mesh.indices[i].normal_index + 1]);
						_data.push_back(attrib.normals[3 * shapes[s].mesh.indices[i].normal_index + 2]);
					}

					if (attrib.texcoords.size() == 0) {
						_data.push_back(0.0f);
						_data.push_back(0.0f);
					} else {
						_data.push_back(attrib.texcoords[2 * shapes[s].mesh.indices[i].texcoord_index]);
						_data.push_back(attrib.texcoords[2 * shapes[s].mesh.indices[i].texcoord_index + 1]);
					}

                    auto material_id = shapes[s].mesh.material_ids[(i - (i % 3)) / 3];
                    
                    auto search = material_indices.find(material_id);
                    if (search != material_indices.end()) {
                        search->second.push_back(index_count);
                    } else {
                        if (material_id < materials.size()) {
                            auto in_tex = _material_groups.insert({material_id, {0, 0, nullptr}});
                            
                            if (materials[material_id].diffuse_texname.length() > 0) {
                                in_tex.first->second.texture = load_texture(d.generic_string().c_str(), materials[material_id].diffuse_texname);
                            }
                            
                            auto in = material_indices.insert({material_id, {}});
                            
                            in.first->second.push_back(index_count);
                        }
                    }
                    
					//indices.push_back(index_count);
					index_count++;

				}
			}

            for (auto& i : material_indices) {
                size_t offset = indices.size();
                indices.reserve(indices.size() + i.second.size());
                indices.insert(indices.end(), i.second.begin(), i.second.end());
                
                auto search = _material_groups.find(i.first);
                
                if (search != _material_groups.end()) {
                    search->second.offset = offset;
                    search->second.length = i.second.size();
                }
            }

		}
        
        std::shared_ptr<azure::Texture> sceneTesselation::load_texture(std::string path, std::string name) {
            fipImage image;
            
            std::string full_path = path + name;
            
            auto search = _textures_loaded.find(full_path);
            
            if (search != _textures_loaded.end()) {
                return search->second;
            }
            
            // FIXME: Use system loaders rather than FreeImage.
            if (!image.load((path + name).c_str())) {
                std::stringstream strm;
                strm << "Error: Scene failed to load texture " << name;
                throw std::runtime_error(strm.str());
            }
            image.adjustGamma(0.4545);
            //image.flipVertical();
            
            unsigned int width = image.getWidth();
            unsigned int height = image.getHeight();
            
            std::shared_ptr<azure::Texture> texture = std::make_shared<azure::Texture>();
            texture->setParam(GL_TEXTURE_WRAP_S, GL_REPEAT);
            texture->setParam(GL_TEXTURE_WRAP_T, GL_REPEAT);
            texture->setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            texture->setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            _glException();
            texture->createTexture(scarlet::FreeImagePlusToGLInternalFormat(image), width, height, scarlet::FreeImagePlusToGLFormat(image), scarlet::FreeImagePlusToGLType(image), image.accessPixels());
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            _glException();
            
            _textures_loaded.insert({full_path, texture});
            
            return texture;
        }

	}
}
