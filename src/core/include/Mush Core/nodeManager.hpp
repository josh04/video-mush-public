#ifndef NODE_MANAGER_HPP
#define NODE_MANAGER_HPP

#include <unordered_map>
#include <map>
#include <memory>
#include "processNode.hpp"
#include "initNode.hpp"
#include "opencl.hpp"

#define NODE_NAME_LENGTH 255

namespace mush {
	namespace node {
		struct info {
			uint32_t id;
			//uint32_t name_mem_length;
			char name[NODE_NAME_LENGTH];
			bool is_inited;
			bool is_gui;
		};

		struct node_storage {
			std::shared_ptr<mush::processNode> pointer;
			uint32_t node_type;
		};

		class type_storage {
		public:
			type_storage(uint32_t type, std::string name) {
				this->type = type;
				int len = (name.length() < NODE_NAME_LENGTH) ? name.length() : NODE_NAME_LENGTH;
				strncpy(this->name, name.c_str(), len);
			}

			uint32_t type;
			char name[NODE_NAME_LENGTH];
		};

		class processor;
		class manager : public std::enable_shared_from_this<manager> {
		public:
			manager();

			~manager();

			uint32_t get_total_node_count() const;
			uint32_t get_total_processor_count() const;
			uint32_t get_processor_node_count(uint32_t processor_id) const;
			std::vector<uint32_t> get_all_processors() const;
			std::vector<uint32_t> get_all_nodes() const;

			uint32_t register_node_type(std::string name);
			const char * get_node_name(uint32_t node_id) const;
			uint32_t get_node_types_count() const;
			std::vector<type_storage> get_node_types() const;

			uint32_t create_default_node(uint32_t node_type_id);

			uint32_t add_external_node(std::shared_ptr<mush::processNode> node, uint32_t node_type_id);

			std::shared_ptr<mush::processNode> get_process_node(const uint32_t node_id) const;
			std::shared_ptr<mush::initNode> get_init_node(const uint32_t node_id) const;
			std::shared_ptr<mush::ringBuffer> get_ring_node(const uint32_t node_id) const;

			template <typename T> std::shared_ptr<T> get_cast_node(const uint32_t node_id) const {
				auto search = _nodes.find(node_id);
				if (search != _nodes.end()) {
					return std::dynamic_pointer_cast<T>(search->second.pointer);
				}
				return nullptr;
			}

			void add_node_link(uint32_t node_id, uint32_t node_link_id);
			void add_node_links(uint32_t node_id, std::initializer_list<uint32_t> node_link_ids);
			std::vector<uint32_t> get_linked_nodes(uint32_t node_id) const;

			void delete_node(const uint32_t node_id);

			void clear();

			uint32_t create_processor();
			void delete_processor(const uint32_t processor_id);
			std::shared_ptr<mush::node::processor> get_processor(const uint32_t processor_id) const;

		private:
			//std::shared_ptr<mush::opencl> _context = nullptr;

			std::unordered_map<uint32_t, node_storage> _nodes;
			std::unordered_map<uint32_t, std::shared_ptr<processor>> _processors;
			std::unordered_map<uint32_t, std::vector<uint32_t>> _node_links;
			std::unordered_map<uint32_t, type_storage> _node_types;

			uint32_t _node_count = 0;
			uint32_t _processor_count = 0;
			uint32_t _node_type_count = 0;

			uint32_t _fixed_exposure_type = -1;
			uint32_t _fisheye2equirectangular = -1;
		};
	}
}

#endif
