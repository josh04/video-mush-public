#ifndef RASTERISER_SCENE_HPP
#define RASTERISER_SCENE_HPP

#include <vector>
#include <memory>
#include "indexGLObject.hpp"

namespace mush {
	namespace raster {
		
		class scene : public azure::indexGLObject {
		public:
			scene();
			scene(const scene & other);
			~scene();

			void init(const char * model_path, const float model_scale);

		private:
			void load_model(const char * model_path, const float model_scale);
			void load_model_new(const char * model_path);
		};
	}
}

#endif
