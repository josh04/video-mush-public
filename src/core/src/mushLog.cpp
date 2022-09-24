//
//  mushLog.cpp
//  mush-core
//
//  Created by Josh McNamee on 04/10/2016.
//  Copyright © 2016 josh04. All rights reserved.
//

#include <stdint.h>

#include "mushLog.hpp"

#include <mutex>
#include <queue>
#include <condition_variable>
#include <memory>
#include <vector>
#include <string>
#include <queue>

std::mutex _global_log_lock;
std::queue<std::string> _global_log_buffer;
std::shared_ptr<std::condition_variable> _global_wait_cond = nullptr;

extern "C" {

	bool getLog(char * output, uint64_t size) {
		std::unique_lock<std::mutex> uniqueLock(_global_log_lock);

		if (_global_wait_cond == nullptr) {
			_global_wait_cond = std::make_shared<std::condition_variable>();
		}
		_global_wait_cond->wait_for(uniqueLock, std::chrono::milliseconds(20), [&]() {return !_global_log_buffer.empty(); });
		if (_global_log_buffer.empty()) {
			return false;
		}
		std::string retur = _global_log_buffer.front();

		snprintf(output, size, "%s", retur.substr(0, size - 1).c_str());

		if (retur.length() > size) {
			_global_log_buffer.front() = retur.substr(size, retur.length());
		} else {
			_global_log_buffer.pop();
		}

		return true;
	}

	void putLog(std::string log) {
		std::lock_guard<std::mutex> glock(_global_log_lock);
		_global_log_buffer.push(log);
		if (_global_wait_cond == nullptr) {
			_global_wait_cond = std::make_shared<std::condition_variable>();
		}
		_global_wait_cond->notify_all();
	}

	void endLog() {
		_global_wait_cond = nullptr;
	}


	std::mutex _global_table_row_names_lock;
	std::mutex _global_table_row_lock;
	std::queue<std::vector<mush::metric_value>> _global_table_row_buffer;
	bool new_table_row_names;
	std::vector<std::string> _global_table_row_names;
	std::shared_ptr<std::condition_variable> _global_table_row_wait_cond = nullptr;


	MUSHEXPORTS_API bool newRowNames(uint32_t * count) {
		std::unique_lock<std::mutex> uniqueLock(_global_table_row_names_lock);
		*count = _global_table_row_names.size();
		return new_table_row_names;
	}


	MUSHEXPORTS_API void getRowNames(char ** names, uint32_t count, size_t max_string_length) {
		std::unique_lock<std::mutex> uniqueLock(_global_table_row_names_lock);

		for (int i = 0; i < _global_table_row_names.size() && i < count; ++i) {
			snprintf(names[i], max_string_length, "%s", _global_table_row_names[i].c_str());
		}
		new_table_row_names = false;
	}

	MUSHEXPORTS_API void getRowName(char * name, uint32_t index, size_t max_string_length) {
		std::unique_lock<std::mutex> uniqueLock(_global_table_row_names_lock);

		snprintf(name, max_string_length, "%s", _global_table_row_names[index].c_str());

		new_table_row_names = false;
	}

	MUSHEXPORTS_API void setRowNames(std::vector<std::string> names) {
		std::unique_lock<std::mutex> uniqueLock(_global_table_row_names_lock);
		new_table_row_names = true;
		_global_table_row_names = names;
	}

	MUSHEXPORTS_API uint32_t getRowCount() {
		std::unique_lock<std::mutex> uniqueLock(_global_table_row_lock);

		if (_global_table_row_wait_cond == nullptr) {
			_global_table_row_wait_cond = std::make_shared<std::condition_variable>();
		}
		_global_table_row_wait_cond->wait_for(uniqueLock, std::chrono::milliseconds(20), [&]() {return !_global_table_row_buffer.empty(); });
		if (_global_table_row_buffer.empty()) {
			return 0;
		}

		return _global_table_row_buffer.front().size();
	}

	MUSHEXPORTS_API bool getRow(mush::metric_value * output, uint32_t size) {
		std::unique_lock<std::mutex> uniqueLock(_global_table_row_lock);

		auto& retur = _global_table_row_buffer.front();

		for (int i = 0; i < retur.size() && i < size; ++i) {
			output[i] = retur[i];
		}

		_global_table_row_buffer.pop();
		return true;
	}

	void addRow(const std::vector<mush::metric_value>& log) {
		std::lock_guard<std::mutex> glock(_global_table_row_lock);
		_global_table_row_buffer.push(log);
		if (_global_table_row_wait_cond == nullptr) {
			_global_table_row_wait_cond = std::make_shared<std::condition_variable>();
		}
		_global_table_row_wait_cond->notify_all();
	}

	void endRow() {
		_global_table_row_wait_cond = nullptr;
	}


}
