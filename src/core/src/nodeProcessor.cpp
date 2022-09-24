#include "nodeProcessor.hpp"
#include "ringBuffer.hpp"

namespace mush {
	namespace node {
		processor::processor(std::shared_ptr<mush::node::manager> manager) : imageProcessor(), _manager(manager) {
			_quit = false;
		}

		processor::~processor() {

		}

		void processor::init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) {
			_context = context;

			for (auto& p : _nodes) {
				auto links = _manager->get_linked_nodes(p);
				if (links.size() > 0) {
					auto pr = _manager->get_init_node(p); 
					if (pr != nullptr) {
						std::vector<std::shared_ptr<ringBuffer>> inits;
						
						for (auto& l : links) {
							auto r = _manager->get_ring_node(l);
							if (r != nullptr) {
								inits.push_back(r);
							}
						}

						switch (inits.size()) {
						case 0:
							pr->init(context, {});
							break;
						case 1:
							pr->init(context, { inits[0] });
							break;
						case 2:
							pr->init(context, { inits[0], inits[1] });
							break;
						case 3:
							pr->init(context, { inits[0], inits[1], inits[2] });
							break;
						case 4:
							pr->init(context, { inits[0], inits[1], inits[2], inits[3] });
							break;
						}
					}
				} else {
					auto pr = _manager->get_init_node(p);
					if (pr != nullptr) {
						pr->init(context, {});
					}

				}
			}
		}

		void processor::process() {
				auto& p = _process_queue.front();
				auto pr = _manager->get_process_node(p);
				if (pr != nullptr) {
					pr->process();
				}
				_process_queue.pop_front();
		}

		void processor::go() {
			while (_process_queue.size() > 0 && !_quit) {
				process();
			}
		}

		void processor::force_quit() {
			quit();
			for (auto& node : _nodes) {
				auto ptr = std::dynamic_pointer_cast<mush::ringBuffer>(_manager->get_process_node(node));
				if (ptr != nullptr) {
					ptr->kill();
				}
			}
		}

		void processor::add_node(uint32_t node_id) {
			_nodes.push_back(node_id);
		}

		void processor::add_node(std::initializer_list<uint32_t> node_ids) {
			_nodes.insert(_nodes.end(), node_ids.begin(), node_ids.end());
		}

		const std::vector<std::shared_ptr<mush::ringBuffer>> processor::getBuffers() const {
			return {};
		}

		std::vector<std::shared_ptr<mush::guiAccessible>> processor::getGuiBuffers() {
			return {};
		}

		uint32_t processor::get_node_count() const {
			return _nodes.size();
		}

		std::vector<uint32_t> processor::get_all_nodes() const {
			return _nodes;
		}
	}
}