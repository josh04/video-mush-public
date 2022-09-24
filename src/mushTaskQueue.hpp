#ifndef MUSHTASKQUEUE_HPP
#define MUSHTASKQUEUE_HPP

#include <deque>
#include "ConfigStruct.hpp"

namespace mush {
	class mushTaskQueue {
	public:
		mushTaskQueue();
		~mushTaskQueue();

		void pushTask(mush::config& c);
		mush::config popTask();

		size_t count() const;

		void clear();
	private:
		std::deque<mush::config> _tasks;
	};
}

#endif

