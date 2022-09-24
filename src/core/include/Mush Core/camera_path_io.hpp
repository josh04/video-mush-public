//
//  camera_path_io.hpp
//  video-mush
//
//  Created by Josh McNamee on 09/12/2016.
//
//

#ifndef camera_path_io_hpp
#define camera_path_io_hpp

#include "mush-core-dll.hpp"
#include <vector>
#include <azure/glm/glm.hpp>

namespace mush {
    namespace camera {
        struct camera_data_struct {
            glm::vec3 location;
            float theta;
            float phi;
            float fov;
            float aspect;
        };

        struct camera_path_node {
            int node_tick;
            camera_data_struct camera;
        };

		class MUSHEXPORTS_API camera_path_manager {
		public:
			camera_path_manager(std::vector<camera_path_node> camera_positions, float speed_factor);
			~camera_path_manager();

			camera_data_struct get_next_camera_position();

			void reset();
			bool is_finished() const;
			void finish();
            
            std::vector<camera_path_node> get_camera_path() const {
                return _camera_positions;
            }
            
		private:
			size_t _node_position = 0;
			size_t _tick = 0;
			float _speed_factor = 1.0f;

			bool _is_finished = false;

			std::vector<camera_path_node> _camera_positions;
		};
        
        MUSHEXPORTS_API void write_camera_path_json(const char * filename, const std::vector<camera_path_node>& path);
        MUSHEXPORTS_API void write_camera_path(const char * filename, const std::vector<camera_path_node>& path);
        MUSHEXPORTS_API bool read_camera_path_json(const char * filename, std::vector<camera_path_node>& path);
		MUSHEXPORTS_API bool read_camera_path(const char * filename, std::vector<camera_path_node>& path, bool legacy_par_format = false);
    }
}

#endif /* camera_path_io_hpp */
