//
//  camera.cpp
//  video-mush
//
//  Created by Josh McNamee on 09/12/2016.
//
//

#include "camera_base.hpp"

namespace mush {
    namespace camera {
        base::base() : base(glm::vec3(0.0, 0.0, 0.0), 0.0, 0.0, 75.0f, 16.0/9.0) {
        }
        
        base::base(glm::vec3 location, float theta, float phi, float fov, float aspect) : camera_location(location), theta(theta), phi(phi), fov(fov), aspect(aspect) {
            update_matrices();
        }
        
        base::~base() {
            
        }
        
        void base::add_location(glm::vec3 add_location) {
            move_camera(camera_location + add_location, theta, phi);
        }
        
        void base::add_theta(float add_theta) {
            move_camera(camera_location, theta + add_theta, phi);
        }
        
        void base::add_phi(float add_phi) {
            move_camera(camera_location, theta, phi + add_phi);
        }
        
        void base::add_fov(float add_fov) {
            set_fov(fov + add_fov);
        }
        
        void base::set_fov(float fov) {
            std::lock_guard<std::mutex> lock(camera_mutex);
            this->fov = fov;
            matrices_need_updating = true;

			track_changes_flag.exchange(true);
        }

		void base::build_additional_shift_matrix(glm::mat4 m) {
			std::lock_guard<std::mutex> lock(camera_mutex);

			glm::mat4 rotation(1.0);
			rotation = glm::rotate(rotation, theta-90.0f, glm::vec3(0, 1, 0));

			additional_shift_matrix = glm::transpose(rotation * m);
			matrices_need_updating = true;

			track_changes_flag.exchange(true);
		}

		void base::build_additional_displacement(glm::vec3 loc) {
			std::lock_guard<std::mutex> lock(camera_mutex);

			glm::mat4 rotation(1.0);
			rotation = glm::rotate(rotation, (float)theta, glm::vec3(0, 1, 0));

			additional_displacement = glm::vec3(rotation * glm::vec4(loc, 0.0f));
			matrices_need_updating = true;

			track_changes_flag.exchange(true);
        }
        
        void base::set_additional_shift_matrix(glm::mat4 m) {
            std::lock_guard<std::mutex> lock(camera_mutex);
            
            additional_shift_matrix = m;
            matrices_need_updating = true;
            
            track_changes_flag.exchange(true);
        }
        
        void base::set_additional_displacement(glm::vec3 loc) {
            std::lock_guard<std::mutex> lock(camera_mutex);
            
            additional_displacement = loc;
            matrices_need_updating = true;
            
            track_changes_flag.exchange(true);
        }
        
        void base::set_aspect(float aspect) {
            std::lock_guard<std::mutex> lock(camera_mutex);
            this->aspect = aspect;
            matrices_need_updating = true;
            
            track_changes_flag.exchange(true);
        }

		void base::move_camera(glm::vec3 new_location) {
			std::lock_guard<std::mutex> lock(camera_mutex);
			camera_location = new_location;
			matrices_need_updating = true;

			track_changes_flag.exchange(true);
		}
        
        void base::move_camera(glm::vec3 new_location, float new_theta, float new_phi) {
            std::lock_guard<std::mutex> lock(camera_mutex);
            camera_location = new_location;
            theta = new_theta;
            phi = new_phi;

			if (_is_spherical) {
				phi = 0.0f;
			}

            matrices_need_updating = true;
            
            track_changes_flag.exchange(true);
        }
        
        void base::update_matrices() {
            std::lock_guard<std::mutex> lock(camera_mutex);
            if (matrices_need_updating) {
                glm::mat4 rotation(1.0);
                
                rotation = glm::rotate(rotation, (float)theta, glm::vec3(0,1,0));
                rotation = glm::rotate(rotation, (float)phi, glm::vec3(0,0,1));
                
                camera_gaze = glm::vec3(1, 0, 0);
                camera_gaze = glm::mat3(rotation) * camera_gaze;
                
                glm::vec3 up(0, 1, 0);
                up = glm::mat3(rotation) * up;
                
                const glm::vec3 w = -camera_gaze;
                const glm::vec3 u = glm::cross(up, w);
                camera_up = glm::cross(w, u);

				camera_gaze = glm::vec3(additional_shift_matrix * glm::vec4(camera_gaze, 0.0f));
				up = glm::vec3(additional_shift_matrix * glm::vec4(up, 0.0f));
                
                P = glm::perspective(fov, aspect, 0.1f, 10000.0f); // projection
                
                V = glm::lookAt(camera_location + additional_displacement, camera_gaze + camera_location + additional_displacement, camera_up); // view
                
                M = glm::mat4(1.0f); // model
                
                MV = V * M;
                MVP = P * V * M;
                matrices_need_updating = false;
            }
        }
        
        float base::get_theta() const {
            return theta;
        }
        
        float base::get_phi() const {
            return phi;
        }
        
        float base::get_fov() const {
            return fov;
        }
        
        float base::get_aspect() const {
            return aspect;
        }
        
        glm::vec3 base::get_location() const {
            return camera_location;
        }
        
        glm::vec3 base::get_gaze() {
            update_matrices();
            return camera_gaze;
        }
        
        glm::vec3 base::get_up() {
            update_matrices();
            return camera_up;
        }
        
        const glm::mat4& base::get_model() {
            update_matrices();
            return M;
        }
        
        const glm::mat4& base::get_view() {
            update_matrices();
            return V;
        }
        
        const glm::mat4& base::get_projection() {
            update_matrices();
            return P;
        }
        
        const glm::mat4& base::get_mv() {
            update_matrices();
            return MV;
        }
        
        const glm::mat4& base::get_mvp() {
            update_matrices();
            return MVP;
        }

		glm::mat4 base::get_additional_shift_matrix() {
			return additional_shift_matrix;
			update_matrices();
		}

		glm::vec3 base::get_additional_displacement() {
			update_matrices();
			return additional_displacement;
		}

		void base::push_temporary_position(glm::vec3 l, float t, float p, float f) {
			std::lock_guard<std::mutex> lock(camera_mutex);

			stashed_location = camera_location;
			stashed_theta = theta;
			stashed_phi = phi;
			stashed_fov = fov;

			camera_location = l;
			theta = t;
			phi = p;
			fov = f;

			matrices_need_updating = true;
		}

		void base::pop_temporary_position() {
			std::lock_guard<std::mutex> lock(camera_mutex);

			camera_location = stashed_location;
			theta = stashed_theta;
			phi = stashed_phi;
			fov = stashed_fov;

			matrices_need_updating = true;
		}
        
    }
}
