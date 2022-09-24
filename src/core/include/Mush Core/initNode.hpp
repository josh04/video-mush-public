#ifndef INITNODE_HPP
#define INITNODE_HPP

#include <memory>

namespace mush {
	class opencl;
	class ringBuffer;

	class initNode {
	public:

		virtual void init(std::shared_ptr<mush::opencl> context, std::shared_ptr<mush::ringBuffer> buffer) {
			init(context, { buffer });
		}

		virtual void init(std::shared_ptr<mush::opencl> context, const std::initializer_list<std::shared_ptr<mush::ringBuffer>> buffers) = 0;

	private:

	};
}

#endif
