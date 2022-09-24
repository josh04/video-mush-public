#ifndef NODE_PROCESSOR_HPP
#define NODE_PROCESSOR_HPP

#include <atomic>
#include <deque>
#include <map>

#include "imageProcessor.hpp"
#include "nodeManager.hpp"

namespace mush {
	namespace node {
		class processor : public imageProcessor {
		public:
			processor(std::shared_ptr<mush::node::manager> manager);
			~processor();
			
			void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) override;
			void process() override;
			void go() override;

			void quit() {
				_quit = true;
			}

			void force_quit();

			void add_node(uint32_t node_id);
			void add_node(std::initializer_list<uint32_t> node_ids);

			const std::vector<std::shared_ptr<mush::ringBuffer>> getBuffers() const override;
			std::vector<std::shared_ptr<mush::guiAccessible>> getGuiBuffers() override;

			uint32_t get_node_count() const;
			std::vector<uint32_t> get_all_nodes() const;
		private:
			std::shared_ptr<mush::opencl> _context = nullptr;
			std::shared_ptr<mush::node::manager> _manager = nullptr;

			std::vector<uint32_t> _nodes;

			std::deque<uint32_t> _process_queue;

			std::atomic_bool _quit;
		};
	}

}

#endif

