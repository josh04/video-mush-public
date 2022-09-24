
#include "mushTaskQueue.hpp"

namespace mush {
	mushTaskQueue::mushTaskQueue() {

	}

	mushTaskQueue::~mushTaskQueue() {

	}

	void mushTaskQueue::pushTask(mush::config& c) {
		_tasks.push_back(c);
	}

	mush::config mushTaskQueue::popTask() {
		auto c = _tasks.front();
		_tasks.pop_front();
		return c;
	}

	size_t mushTaskQueue::count() const {
		return _tasks.size();
	}

	void mushTaskQueue::clear() {
		_tasks.clear();
	}

}