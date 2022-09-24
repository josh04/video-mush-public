//
//  camera.hpp
//  video-mush
//
//  Created by Josh McNamee on 09/12/2016.
//
//

#ifndef camera_hpp
#define camera_hpp

#include "mush-core-dll.hpp"

#ifdef __APPLE__
#include <azure/glm/glm.hpp>
#include <azure/glm/gtc/matrix_transform.hpp>
#include <azure/glm/gtx/transform.hpp>
#else
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#endif

#include <mutex>
#include <atomic>

namespace mush {
    namespace camera {
		class MUSHEXPORTS_API  base {
    public:
        base();
        base(glm::vec3 location, float theta, float phi, float fov, float aspect);
        ~base();
        
        void add_location(glm::vec3 add_location);
        void add_theta(float add_theta);
        void add_phi(float add_phi);
        void add_fov(float add_fov);
        
        void set_fov(float fov);
            
        void build_additional_shift_matrix(glm::mat4 m); // these take the values from the oculus API and correct for them
        void build_additional_displacement(glm::vec3 loc); //
            
		void set_additional_shift_matrix(glm::mat4 m); // these just copy the values
		void set_additional_displacement(glm::vec3 loc); // 
        
        void move_camera(glm::vec3 new_location, float new_theta, float new_phi);
		void move_camera(glm::vec3 new_location);
        
        void set_aspect(float aspect);
        
        float get_theta() const;
        float get_phi() const;
        float get_fov() const;
        float get_aspect() const;
        
        glm::vec3 get_location() const;
        glm::vec3 get_gaze();
        glm::vec3 get_up();
        const glm::mat4& get_model();
        const glm::mat4& get_view();
        const glm::mat4& get_projection();
        const glm::mat4& get_mv();
        const glm::mat4& get_mvp();

		glm::mat4 get_additional_shift_matrix();
		glm::vec3 get_additional_displacement();
        
        bool check_track_changes_flag() {
            return track_changes_flag.exchange(false);
        }

		void set_track_changes_flag() {
			track_changes_flag.exchange(true);
		}

		void push_temporary_position(glm::vec3 location, float theta, float phi, float fov);
		void pop_temporary_position();

		void set_spherical(bool spherical) {
			_is_spherical = spherical;
		}
            
        //void add_stereo_shift(float distance);
        //void remove_stereo_shit(float distance);
    protected:
        virtual void update_matrices();

        //void stereo_shift(float distance);
            
		bool _is_spherical = false;
        
        float aspect;
        
        glm::vec3 camera_location, camera_gaze, camera_up;
        float theta, phi;

		glm::vec3 stashed_location;
		float stashed_theta, stashed_phi, stashed_fov;
        
        glm::mat4 M, V, P, MV, MVP;
        
        float fov;
        
        std::mutex camera_mutex;
        bool matrices_need_updating = true;
        
        std::atomic_bool track_changes_flag;

		glm::mat4 additional_shift_matrix;
		glm::vec3 additional_displacement;
    };
    }
}

#endif /* camera_hpp */
