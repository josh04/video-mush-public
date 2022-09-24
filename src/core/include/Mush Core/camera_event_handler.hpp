//
//  camera_event_handler.hpp
//  video-mush
//
//  Created by Josh McNamee on 09/12/2016.
//
//

#ifndef camera_event_handler_hpp
#define camera_event_handler_hpp

#include "mush-core-dll.hpp"
#include <atomic>

#include <azure/eventable.hpp>
#include <azure/events.hpp>
#include <azure/eventkey.hpp>

#include "camera_path_io.hpp"
#include "camera_base.hpp"

namespace mush {
    namespace camera {
        
        class MUSHEXPORTS_API camera_event_handler : public azure::Eventable {
        public:
            camera_event_handler(std::shared_ptr<camera::base> camera);
            ~camera_event_handler();
            
            void frame_tick();
			bool camera_path_finished() const;
            
            bool event(std::shared_ptr<azure::Event> event) override;
            
            void move_camera();
            
			void set_active(bool active) {
				_active.exchange(active);
			}
			void set_mouse_active(bool active) {
				_mouse_active.exchange(active);
			}

			void load_camera_path(const std::string camera_path, float speed_factor, bool radians_to_degrees = false, bool quit_at_end = false);
            void write_camera_path(const std::string camera_path) const;
			void end_camera_path();
            
            std::vector<camera_path_node> get_camera_path() const {
                if (_camera_path != nullptr) {
                    return _camera_path->get_camera_path();
                } else {
                    return {};
                }
            }
            
        private:
			std::mutex _camera_mutex;

            std::atomic_int _tick;
            
            std::shared_ptr<camera::base> _camera;
            
            bool _tracking_mouse = false;
            int _m_x = 0, _m_y = 0;

			std::shared_ptr<camera_path_manager> _camera_path = nullptr;
            
            //std::vector<camera_path_node> _camera_positions;
            std::vector<camera_path_node> _saved_camera_positions;
            
            //float _speed_factor = 1.0f;
            //size_t _camera_position_node = 0;
            
            //size_t _camera_position_tick = 0;
            
            bool _w_pressed = false;
            bool _a_pressed = false;
            bool _s_pressed = false;
            bool _d_pressed = false;
            bool _e_pressed = false;
            bool _x_pressed = false;
            
            bool _o_pressed = false;
            bool _p_pressed = false;

			float _mouse_diff_x = 0.0f;
			float _mouse_diff_y = 0.0f;
            
            bool _shift_pressed = false;
            bool _ctrl_pressed = false;
            bool _reset = false;

			std::atomic_bool _active;
			std::atomic_bool _mouse_active;
            
            bool _quit_at_end = false;

			std::chrono::time_point<std::chrono::high_resolution_clock> _previous_time;
        };
    }
    
}
#endif /* camera_event_handler_hpp */
