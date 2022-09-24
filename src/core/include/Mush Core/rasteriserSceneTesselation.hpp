#ifndef RASTERISER_SCENE_TESSELATION_HPP
#define RASTERISER_SCENE_TESSELATION_HPP

#include <vector>
#include <memory>
#include <unordered_map>
#include "indexGLObject.hpp"
#include "mush-core-dll.hpp"

namespace azure {
    class Texture;
}

namespace mush {
	namespace raster {

		class MUSHEXPORTS_API sceneTesselation : public azure::indexGLObject {
		public:
			sceneTesselation();
			sceneTesselation(const sceneTesselation & other);
			~sceneTesselation();

			void render() override;

			void init(const char * model_path, const float model_scale);
			std::array<float, 3> CalcNormal(float v0[3], float v1[3], float v2[3]);
            
            void enable_depth_program() { _use_depth_program = true; }
            void disable_depth_program() { _use_depth_program = false; }
            
            std::shared_ptr<azure::Program> get_depth_program() const {
                return _depth_program;
            }
            
            std::shared_ptr<azure::Program> get_texture_program() const {
                return _texture_program;
            }

            std::shared_ptr<azure::Texture> load_texture(std::string path, std::string name);

        protected:
            struct material_offset_texture {
                size_t offset;
                size_t length;
                std::shared_ptr<azure::Texture> texture;
            };
            
            std::map<int, material_offset_texture> _material_groups;
            
            std::shared_ptr<azure::Program> _regular_program = nullptr;
            std::shared_ptr<azure::Program> _depth_program = nullptr;
            std::shared_ptr<azure::Program> _texture_program = nullptr;
            
            bool _use_depth_program = false;
		private:
			void load_model(const char * model_path, const float model_scale);
            
            
            std::unordered_map<std::string, std::shared_ptr<azure::Texture>> _textures_loaded;
		};
	}
}

#endif
