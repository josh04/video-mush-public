#ifndef MUSHIMAGE_HPP
#define MUSHIMAGE_HPP

#include "mush-core-dll.hpp"
#include "opencl.hpp"

#include <memory>

namespace cl {
    class Image2D;
    class Image;
    class Buffer;
}

namespace azure {
	class Framebuffer;
}

namespace mush {
    
    
    class MUSHEXPORTS_API buffer {
    public:


		class MUSHEXPORTS_API parameters {
		public:
			parameters() {

			}

			~parameters() {

			}

			bool has_camera_position = false;
			cl_float3 camera_position, theta_phi_fov;
		};

        enum class type {
            cl_image,
            cl_buffer,
			gl_framebuffer,
            host_pointer,
            empty
        };

        buffer() {
            _t = type::empty;
            _content = nullptr;
            _parameters = std::make_shared<parameters>();
        }
        
        buffer(cl::Image2D image) {
            _t = type::cl_image;
            _content = image();
            _parameters = std::make_shared<parameters>();
        }
        
        buffer(cl::ImageGL image) {
            _t = type::cl_image;
            _content = image();
            _parameters = std::make_shared<parameters>();
        }

		buffer(std::shared_ptr<azure::Framebuffer> texture) {
			_t = type::gl_framebuffer;
			_content = texture.get();
			_parameters = std::make_shared<parameters>();
		}
        
        buffer(cl::Buffer buf) {
            _t = type::cl_buffer;
            _content = buf();
            _parameters = std::make_shared<parameters>();
        }
        
        buffer(void * buf) {
            _t = type::host_pointer;
            _content = buf;
            _parameters = std::make_shared<parameters>();
        }
        
        buffer(const void * buf) {
            _t = type::host_pointer;
            _content = (void *)buf;
            _parameters = std::make_shared<parameters>();
        }
        
        buffer(const buffer& rhs) {
            _t = rhs.get_type();
            _content = rhs._get_raw();
            _parameters = rhs._get_parameters();
        }
        
        ~buffer() {}
        
        bool operator==(const void * &rhs) const {
            return _content == rhs;
        }
        
        bool operator!=(const void * &rhs) const {
            return _content != rhs;
        }
        bool operator==(const std::nullptr_t &rhs) const {
            return _t == type::empty;
        }
        
        bool operator!=(const std::nullptr_t &rhs) const {
            return _t != type::empty;
        }
        
        cl_mem get_image() const {
            if (_t != type::cl_image) {
                throw std::runtime_error("Called get_image on a non-image buffer.");
            }
            return (cl_mem)_content;
        }

		azure::Framebuffer * get_gl_framebuffer() const {
			if (_t != type::gl_framebuffer) {
				throw std::runtime_error("Called get_image on a non-image buffer.");
			}
			return (azure::Framebuffer *)_content;
		}
        
        cl_mem get_buffer() const {
            if (_t != type::cl_buffer) {
                throw std::runtime_error("Called get_buffer on a non-buffer buffer.");
            }
            return (cl_mem)_content;
        }
        
        void * get_pointer() const {
            if (_t != type::host_pointer) {
                throw std::runtime_error("Called get_pointer on a non-pointer buffer.");
            }
            return _content;
        }
        
        void copy_parameters(const buffer& rhs) {
            if (rhs.has_camera_position()) {
                set_camera_position(rhs.get_camera_position(), rhs.get_theta_phi_fov());
            } else {
                set_no_camera_position();
            }
        }
        
        void set_camera_position(cl_float3 camera_position, cl_float3 theta_phi_fov) {
            _parameters->has_camera_position = true;
            _parameters->camera_position = camera_position;
            _parameters->theta_phi_fov = theta_phi_fov;
        }
        
        bool has_camera_position() const {
            return _parameters->has_camera_position;
        }
        
        
        void set_no_camera_position() {
            _parameters->has_camera_position = false;
        }
        
        cl_float3 get_camera_position() const {
            return _parameters->camera_position;
        }
        
        cl_float3 get_theta_phi_fov() const {
            return _parameters->theta_phi_fov;
        }
        
        type get_type() const {
            return _t;
        }
        
        void * _get_raw() const {
            return _content;
        }

		std::shared_ptr<parameters> _get_parameters() const {
			return _parameters;
		}

        
    private:
        type _t;
        void * _content;
        
        std::shared_ptr<mush::buffer::parameters> _parameters;
        
    };
}

#endif
