//
//  camera_path_io.cpp
//  video-mush
//
//  Created by Josh McNamee on 09/12/2016.
//
//

#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/filesystem.hpp>

#include "checkExists.hpp"

#include "mushLog.hpp"
#include "camera_path_io.hpp"

extern "C" {
    #include "json/json.h"
}

namespace mush {
	namespace camera {

		float HermiteInterpolate(
			float y0, float y1,
			float y2, float y3,
			float mu,
			float tension,
			float bias)
		{
			float m0, m1, mu2, mu3;
			float a0, a1, a2, a3;

			mu2 = mu * mu;
			mu3 = mu2 * mu;
			m0 = (y1 - y0)*(1 + bias)*(1 - tension) / 2;
			m0 += (y2 - y1)*(1 - bias)*(1 - tension) / 2;
			m1 = (y2 - y1)*(1 + bias)*(1 - tension) / 2;
			m1 += (y3 - y2)*(1 - bias)*(1 - tension) / 2;
			a0 = 2 * mu3 - 3 * mu2 + 1;
			a1 = mu3 - 2 * mu2 + mu;
			a2 = mu3 - mu2;
			a3 = -2 * mu3 + 3 * mu2;

			return (a0*y1 + a1*m0 + a2*m1 + a3*y2);
		}

		camera_path_node iterate_camera_path(const std::vector<camera_path_node>& camera_positions, size_t camera_position_node, const size_t& camera_position_tick, const float speed_factor) {
			//if (camera_positions.size() > 0) {
				camera_path_node p1, p2, p3, p4;
				p2 = camera_positions[camera_position_node];

				int previous_ticks = p2.node_tick / speed_factor;
				int ticks = camera_position_tick - previous_ticks;
				int next_ticks = 0;

				if (camera_position_node > 0) {
					p1 = camera_positions[camera_position_node - 1];
				} else {
					p1 = p2;
				}

				if (camera_position_node + 1 < camera_positions.size()) {
					p3 = camera_positions[camera_position_node + 1];
					next_ticks = p3.node_tick / speed_factor;
				} else {
					p3 = p2;
					next_ticks = 0;
				}

				if (camera_position_node + 2 < camera_positions.size()) {
					p4 = camera_positions[camera_position_node + 2];
				} else {
					p4 = p3;
				}

				float mu = ticks / (float)(next_ticks - previous_ticks);
				float tension = 0.0f;
				float bias = 0.0f;

				glm::vec3 new_location;
				float theta, phi, fov, aspect;

				new_location.x = HermiteInterpolate(p1.camera.location.x, p2.camera.location.x, p3.camera.location.x, p4.camera.location.x, mu, tension, bias);
				new_location.y = HermiteInterpolate(p1.camera.location.y, p2.camera.location.y, p3.camera.location.y, p4.camera.location.y, mu, tension, bias);
				new_location.z = HermiteInterpolate(p1.camera.location.z, p2.camera.location.z, p3.camera.location.z, p4.camera.location.z, mu, tension, bias);
				theta = HermiteInterpolate(p1.camera.theta, p2.camera.theta, p3.camera.theta, p4.camera.theta, mu, tension, bias);
				phi = HermiteInterpolate(p1.camera.phi, p2.camera.phi, p3.camera.phi, p4.camera.phi, mu, tension, bias);
				fov = HermiteInterpolate(p1.camera.fov, p2.camera.fov, p3.camera.fov, p4.camera.fov, mu, tension, bias);
				aspect = HermiteInterpolate(p1.camera.aspect, p2.camera.aspect, p3.camera.aspect, p4.camera.aspect, mu, tension, bias);


				if (ticks + previous_ticks - next_ticks >= 0) {
					camera_position_node++;
					/*
					std::stringstream strm;
					if (camera_position_node < camera_positions.size()) {
						strm << "Camera node: " << camera_position_node << " of " << camera_positions.size() << ".";
					} else {
						strm << "Camera path finished.";
						//camera_positions.clear();
					}
					putLog(strm.str());
					*/
				}
                // packing the position in the tick slot here
				return{ (int)camera_position_node, { new_location, theta, phi, fov, aspect } };
            //}
		}

		camera_path_manager::camera_path_manager(std::vector<camera_path_node> camera_positions, float speed_factor) : _camera_positions(camera_positions), _speed_factor(speed_factor) {

		}

		camera_path_manager::~camera_path_manager() {

		}

		camera_data_struct camera_path_manager::get_next_camera_position() {
			auto out = iterate_camera_path(_camera_positions, _node_position, _tick, _speed_factor);

//			_node_position = out.node;

			std::stringstream strm;

			if (out.node_tick < _camera_positions.size()) {
				if (out.node_tick != _node_position) {
					strm << "Camera node: " << _node_position + 1 << " of " << _camera_positions.size() << ".";
					_node_position = out.node_tick;
				}
			} else {
                strm << "Camera node: " << _camera_positions.size() << " of " << _camera_positions.size() << "." << std::endl;
				strm << "Camera path finished.";
				_is_finished = true;
				//camera_positions.clear();
			}
            if (strm.str().length() > 0) {
                putLog(strm.str());
            }

			_tick += 1;

			return out.camera;
		}

		void camera_path_manager::reset() {
			_tick = 0;
			_node_position = 0;
			_is_finished = false;
		}

		bool camera_path_manager::is_finished() const {
			return _is_finished;
		}

		void camera_path_manager::finish() {
			_is_finished = true;
		}
        
        void write_camera_path_json(const char * filename, const std::vector<camera_path_node>& path) {
            
            JsonNode * json_object = json_mkobject();
            
            JsonNode * node_count = json_mknumber(path.size());
            
            json_append_member(json_object, "node_count", node_count);
            
            JsonNode * node_array = json_mkarray();
            
            std::vector<JsonNode *> json_nodes(path.size());
            
            for (int i = 0; i < json_nodes.size(); ++i) {
                auto& el = json_nodes[i];
                auto& p = path[i];
                el = json_mkobject();
                
                JsonNode * tick;
                JsonNode * loc_x, * loc_y, * loc_z;
                JsonNode * theta, * phi;
                JsonNode * fov, * aspect;
                
                tick = json_mknumber(p.node_tick);
                
                loc_x = json_mknumber(p.camera.location.x);
                loc_y = json_mknumber(p.camera.location.y);
                loc_z = json_mknumber(p.camera.location.z);
                
                theta = json_mknumber(p.camera.theta);
                phi = json_mknumber(p.camera.phi);
                
                fov = json_mknumber(p.camera.fov);
                aspect = json_mknumber(p.camera.aspect);
                
                json_append_member(el, "tick", tick);
                
                json_append_member(el, "loc_x", loc_x);
                json_append_member(el, "loc_y", loc_y);
                json_append_member(el, "loc_z", loc_z);
                
                json_append_member(el, "theta", theta);
                json_append_member(el, "phi", phi);
                
                json_append_member(el, "fov", fov);
                json_append_member(el, "aspect", aspect);
                
                json_append_element(node_array, el);
            }
            
            json_append_member(json_object, "nodes", node_array);
            
            const char * output = json_encode(json_object);
            
            std::string fullPath = checkExists(filename);
            
            std::ofstream out(fullPath.c_str(), std::ios_base::out);
            
            out.write(output, strlen(output));

            json_delete(json_object);
        }
        
        void write_camera_path(const char * filename, const std::vector<camera_path_node>& path) {
            
            
            std::string fullPath = checkExists(filename);
            
            auto output = std::make_shared<std::ofstream>(fullPath.c_str(), std::ios_base::out | std::ios_base::binary);
            
            char ptr[5] = { "PARC" };
            
            output->write(ptr, sizeof(uint8_t) * 4);
            size_t cnt = path.size()*sizeof(camera_path_node);
            
            output->write((const char *)&cnt, sizeof(size_t));
            output->write((const char *)&path[0], cnt);
            
            std::stringstream strm;
            strm << "Wrote " << path.size() << " camera nodes to " << fullPath << ".";
            putLog(strm.str());
        }
        
        bool checkId(const char * ptr, const char * id) {
            if (strlen(ptr) >= 4 && strlen(id) == 4) {
                if (ptr[0] == id[0]) {
                    if (ptr[1] == id[1] && ptr[2] == id[2] && ptr[3] == id[3]) {
                        return true;
                    }
                }
            }
            return false;
        }
        
        bool read_camera_path_json(const char * filename, std::vector<camera_path_node>& path) {
            
            std::ifstream input(filename, std::ios_base::in);
            
            std::string json_string;
            
            input.seekg(0, std::ios::end);
            json_string.reserve(input.tellg());
            input.seekg(0, std::ios::beg);
            
            json_string.assign((std::istreambuf_iterator<char>(input)),
                       std::istreambuf_iterator<char>());
            
            JsonNode * json_object = json_decode(json_string.c_str());
            
            JsonNode * node_count_object = json_find_member(json_object, "node_count");
            
            int node_count = (int)node_count_object->number_;
            
            path.reserve(node_count);
            
            JsonNode * node_array = json_find_member(json_object, "nodes");
            
            for (int i = 0; i < node_count; ++i) {
                JsonNode * el = json_find_element(node_array, i);
                
                camera_path_node node;
                node.node_tick = json_find_member(el, "tick")->number_;
                node.camera.location.x = json_find_member(el, "loc_x")->number_;
                node.camera.location.y = json_find_member(el, "loc_y")->number_;
                node.camera.location.z = json_find_member(el, "loc_z")->number_;
                
                node.camera.theta = json_find_member(el, "theta")->number_;
                node.camera.phi = json_find_member(el, "phi")->number_;
                
//                node.camera.theta = json_find_member(el, "theta")->number_ * 180.0/M_PI;
//                node.camera.phi = json_find_member(el, "phi")->number_ * 180.0/M_PI - 90.0f;
                
                node.camera.fov = json_find_member(el, "fov")->number_;
                node.camera.aspect = json_find_member(el, "aspect")->number_;
                
                path.push_back(node);
            }
            
            std::stringstream strm;
            strm << "Read " << path.size() << " camera nodes from " << filename << ".";
            putLog(strm.str());
            return true;
        }
        
        struct Vec3 { float x, y, z, w; };
        
		struct parCameraStruct {
			Vec3 from;
			Vec3 dir; // isn't updated manually
			Vec3 tup; // isn't updated manually
			float theta;
			float phi;
			float fov;
			unsigned int width;
			unsigned int height;
		};

		struct parCameraPathNode {
			int node;
			parCameraStruct camera;
		};
        
//        bool read_camera_path_json(const char * filename, std::vector<camera_path_node>& path) {
        
//        }
        
        bool read_camera_path(const char * filename, std::vector<camera_path_node>& path, bool legacy_par_format) {
            
            auto input = std::make_shared<std::ifstream>(filename, std::ios_base::in | std::ios_base::binary);
            
            char ptr[5] = "PARC";
            char test[5] = "0000";
            input->read(test, sizeof(uint8_t) * 4);
            
            if (!checkId(ptr, test)) {
                std::stringstream strm;
                strm << "Failed camera path read: bad magic number" << test;
                putLog(strm.str());
                putLog("Failed camera path read: bad magic number");
                return false;
            }
            
            size_t cnt;
            
            
            
            input->read((char *)&cnt, sizeof(size_t));
            
			size_t node_size = sizeof(camera_path_node);
			char * data_ptr;
			
			if (legacy_par_format) {
				node_size = sizeof(parCameraPathNode);
			}

            for (int i = 0; i < cnt/ node_size; ++i) {
                camera_path_node p;
				parCameraPathNode ap;
				data_ptr = (char *)&p;
				if (legacy_par_format) {
					data_ptr = (char *)&ap;
				}
                input->read(data_ptr, node_size);
                if (input->gcount() != node_size) {
                    putLog("Failed camera path read: failed to read");
                    return false;
                }
				if (legacy_par_format) {
					p.node_tick = ap.node;
                    p.camera.location.x = ap.camera.from.x;
                    p.camera.location.y = ap.camera.from.y;
                    p.camera.location.z = ap.camera.from.z;
					p.camera.theta = ap.camera.theta;
					p.camera.phi = ap.camera.phi;
					p.camera.fov = ap.camera.fov;
					p.camera.aspect = ap.camera.width / (float)ap.camera.height;
				}
                path.push_back(p);
            }
            
            std::stringstream strm;
            strm << "Read " << path.size() << " camera nodes from " << filename << ".";
            putLog(strm.str());
            return true;
        }
    }
}
