#include "nodeManager.hpp"
#include "nodeProcessor.hpp"
#include "fixedExposureProcess.hpp"
#include "fisheye2EquirectangularProcess.hpp"

namespace mush {
	namespace node {
		manager::manager() {
			_fixed_exposure_type = register_node_type("Fixed Exposure");
			_fisheye2equirectangular = register_node_type("Fisheye 2 Equirectangular");
		}

		manager::~manager() {

		}

		uint32_t manager::get_total_node_count() const {
			return _nodes.size();
		}

		uint32_t manager::get_total_processor_count() const {
			return _processors.size();
		}
		 
		uint32_t manager::get_processor_node_count(uint32_t processor_id) const {
			auto p = get_processor(processor_id);
			return p->get_node_count();
		}

		std::vector<uint32_t> manager::get_all_processors() const {
			std::vector<uint32_t> processors;
			for (auto& p : _processors) {
				processors.push_back(p.first);
			}
			return processors;
		}

		std::vector<uint32_t> manager::get_all_nodes() const {
			std::vector<uint32_t> nodes;
			for (auto& p : _nodes) {
				nodes.push_back(p.first);
			}
			return nodes;
		}

		uint32_t manager::register_node_type(std::string name) {
			auto ret = _node_types.insert({ _node_type_count, type_storage{_node_type_count, name} });
			_node_type_count++;
			return ret.first->first;
		}

		const char * manager::get_node_name(uint32_t node_id) const {
			auto search = _nodes.find(node_id);
			if (search != _nodes.end()) {
				auto search2 = _node_types.find(search->second.node_type);
				if (search2 != _node_types.end()) {
					return search2->second.name;
				}
			}
			return "UNKNOWN";
		}

		uint32_t manager::get_node_types_count() const {
			return _node_types.size();
		}

		std::vector<type_storage> manager::get_node_types() const {
			std::vector<type_storage> types;
			for (auto& t : _node_types) {
				types.push_back(t.second);
			}
			return types;
		}

		uint32_t manager::create_default_node(uint32_t node_type_id) {
			std::shared_ptr<mush::processNode> ptr = nullptr;
			if (node_type_id == _fixed_exposure_type) {
				ptr = std::make_shared<mush::fixedExposureProcess>(0.0f);
			} else if (node_type_id == _fisheye2equirectangular) {
				ptr = std::make_shared<mush::fisheye2EquirectangularProcess>(185.0f, 0.497f, 0.52f);
			}
			if (ptr != nullptr) {
				auto p = _nodes.insert({ _node_count,{ ptr, node_type_id } });
				_node_count++;
				return p.first->first;
			}

			return -1;
		}

		uint32_t manager::add_external_node(std::shared_ptr<mush::processNode> node, uint32_t node_type_id) {
			auto p = _nodes.insert({ _node_count, {node, node_type_id } });
			_node_count++;
			return p.first->first;
		}

		std::shared_ptr<mush::processNode> manager::get_process_node(const uint32_t node_id) const {
			auto search = _nodes.find(node_id);
			if (search != _nodes.end()) {
				return search->second.pointer;
			}
			return nullptr;
		}

		std::shared_ptr<mush::initNode> manager::get_init_node(const uint32_t node_id) const {
			auto search = _nodes.find(node_id);
			if (search != _nodes.end()) {
				return std::dynamic_pointer_cast<mush::initNode>(search->second.pointer);
			}
			return nullptr;
		}

		std::shared_ptr<mush::ringBuffer> manager::get_ring_node(const uint32_t node_id) const {
			auto search = _nodes.find(node_id);
			if (search != _nodes.end()) {
				return std::dynamic_pointer_cast<mush::ringBuffer>(search->second.pointer);
			}
			return nullptr;
		}

		void manager::delete_node(const uint32_t node_id) {
			auto search = _nodes.find(node_id);
			if (search != _nodes.end()) {
				_nodes.erase(node_id);
			}
		}

		void manager::clear() {
			_nodes.clear();
		}

		uint32_t manager::create_processor() {
			auto ptr = std::make_shared<mush::node::processor>(shared_from_this());
			auto p = _processors.insert({ _processor_count, ptr });

			_processor_count++;
			return p.first->first;
		}

		void manager::delete_processor(const uint32_t processor_id) {
			auto search = _processors.find(processor_id);
			if (search != _processors.end()) {
				_processors.erase(processor_id);
			}
		}

		std::shared_ptr<mush::node::processor> manager::get_processor(const uint32_t processor_id) const {
			auto search = _processors.find(processor_id);
			if (search != _processors.end()) {
				return search->second;
			}
			return nullptr;
		}

		void manager::add_node_link(uint32_t node_id, uint32_t node_link_id) {
			add_node_links(node_id, { node_link_id });
		}

		void manager::add_node_links(uint32_t node_id, std::initializer_list<uint32_t> node_link_ids) {
			auto search = _node_links.find(node_id);
			if (search == _node_links.end()) {
				_node_links.insert({ node_id, node_link_ids });
			} else {
				search->second.insert(search->second.end(), node_link_ids.begin(), node_link_ids.end());
			}
		}

		std::vector<uint32_t> manager::get_linked_nodes(uint32_t node_id) const {
			auto search = _node_links.find(node_id);
			if (search != _node_links.end()) {
				return search->second;
			}
			return {};
		}
	}
}