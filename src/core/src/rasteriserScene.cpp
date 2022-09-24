#include <azure/program.hpp>
#include "rasteriserScene.hpp"
#include "tinyobjloader/tiny_obj_loader.h"
#include <boost/filesystem.hpp>
#include "mushLog.hpp"

//#include <assimp/Importer.hpp>

#include <math.h>
#include <array>

namespace mush {
	namespace raster {
		scene::scene() : azure::indexGLObject(nullptr) {

			const char * vert = "shaders/raster.vert";
			const char * frag = "shaders/raster.frag";

			_program = std::make_shared<azure::Program>(vert, frag);
			_glException();
			_program->link();
			_glException();
			_program->use();
			_glException();
		}

		scene::scene(const scene & other) : azure::indexGLObject(other) {

		}

		scene::~scene() {

		}

		void scene::init(const char * model_path, const float model_scale) {

			_layout.push_back(azure::GLObject::Layout("vert", 3, GL_FLOAT, GL_FALSE));
			_layout.push_back(azure::GLObject::Layout("norm", 3, GL_FLOAT, GL_TRUE));
			_layout.push_back(azure::GLObject::Layout("texcoord", 2, GL_FLOAT, GL_FALSE));

			load_model(model_path, model_scale);
			
			index_buffer();
			buffer();
			index_attach();
			attach();
		}

		std::array<float, 3> CalcNormal(float v0[3], float v1[3], float v2[3]) {
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

		void scene::load_model(const char * model_path, const float model_scale) {
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

			shapes[0].mesh.indices;;

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

			// Loop over shapes
			for (size_t s = 0; s < shapes.size(); s++) {

				std::array<float, 3> normals;

				for (int i = 0; i < shapes[s].mesh.indices.size(); ++i) {

					_data.push_back(attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index] * model_scale);
					_data.push_back(attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index+1] * model_scale);
					_data.push_back(attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index+2] * model_scale);

					if (attrib.normals.size() == 0) {
						// Generate normals
						if (i % 3 == 0) {
							float * v1 = &attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index];

							float * v2 = &attrib.vertices[3 * shapes[s].mesh.indices[i+1].vertex_index];

							float * v3 = &attrib.vertices[3 * shapes[s].mesh.indices[i+2].vertex_index];

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
					_data.push_back(attrib.texcoords[2 * shapes[s].mesh.indices[i].texcoord_index]);
					_data.push_back(attrib.texcoords[2 * shapes[s].mesh.indices[i].texcoord_index + 1]);

					indices.push_back(index_count);
					index_count++;

				}

			}


		}

		void scene::load_model_new(const char * model_path) {
			boost::filesystem::path p(model_path);


			//Assimp::Importer importer;
			//aiScene * scene = importer.ReadFile(pFile, aiProcessPreset_TargetRealtime_Quality);

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

			shapes[0].mesh.indices;;

			size_t index_count = 0;
			// Loop over shapes
			for (size_t s = 0; s < shapes.size(); s++) {

				_data.reserve(index_count * 8 + shapes[s].mesh.indices.size() * 8);

				indices.reserve(index_count + shapes[s].mesh.indices.size());

				for (int i = 0; i < shapes[s].mesh.indices.size(); ++i) {

					_data.push_back(attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index]);
					_data.push_back(attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index + 1]);
					_data.push_back(attrib.vertices[3 * shapes[s].mesh.indices[i].vertex_index + 2]);

					_data.push_back(attrib.normals[3 * shapes[s].mesh.indices[i].normal_index]);
					_data.push_back(attrib.normals[3 * shapes[s].mesh.indices[i].normal_index + 1]);
					_data.push_back(attrib.normals[3 * shapes[s].mesh.indices[i].normal_index + 2]);

					_data.push_back(attrib.texcoords[2 * shapes[s].mesh.indices[i].texcoord_index]);
					_data.push_back(attrib.texcoords[2 * shapes[s].mesh.indices[i].texcoord_index + 1]);

					indices.push_back(index_count);
					index_count++;

				}

			}


		}
	}
}
