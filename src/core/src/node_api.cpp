//
//  node_api.cpp
//  mush-core
//
//  Created by Josh McNamee on 17/07/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif

#include <mutex>
#include <deque>
#include <sstream>
#include "node_api.hpp"
#include "nodeManager.hpp"
#include "nodeProcessor.hpp"
#include "fakeWindow.hpp"
#include "tagInGui.hpp"
#include "guiAccessible.hpp"

std::shared_ptr<mush::node::manager> _manager = nullptr;

void node_init();

inline void if_node_init() {
	if (_manager == nullptr) {
		node_init();
	}
}

uint32_t node_get_total_node_count() {
	if_node_init();
	return _manager->get_total_node_count();
}

uint32_t node_get_total_processor_count() {
	if_node_init();
	return _manager->get_total_processor_count();
}

uint32_t node_get_processor_node_count(uint32_t processor_id) {
	if_node_init();
	return _manager->get_processor_node_count(processor_id);
}

void node_get_all_processors(uint32_t * output, uint32_t count) {
	if_node_init();
	auto processors = _manager->get_all_processors();
	if (processors.size() >= count && processors.size() > 0) {
		memcpy(output, &processors[0], count * sizeof(uint32_t));
	}
}

void node_get_all_nodes(mush::node::info * output, uint32_t count) {
	if_node_init();
	auto nodes = _manager->get_all_nodes();
	if (nodes.size() >= count && nodes.size() > 0) {
		for (int i = 0; i < count; ++i) {

			output[i].id = nodes[i];
			/*
			if (NODE_NAME_LENGTH < output[i].name_mem_length) {
				output[i].name_mem_length = NODE_NAME_LENGTH;
			}
			*/
			auto name = _manager->get_node_name(nodes[i]);
			strncpy(output[i].name, name, NODE_NAME_LENGTH);

			output[i].is_gui = false;
			output[i].is_inited = node_is_inited(nodes[i]);
		}
	}
}

bool node_get_all_nodes_from_processor(uint32_t processor_id, uint32_t * output, uint32_t count) {
	if_node_init();
	auto processor = _manager->get_processor(processor_id);
	if (processor != nullptr) {
		auto nodes = processor->get_all_nodes();
		if (nodes.size() >= count && nodes.size() > 0) {
			memcpy(output, &nodes[0], count * sizeof(uint32_t));
		}
		return true;
	} else {
		return false;
	}
}

uint32_t node_get_node_type_count() {
	if_node_init();
	return _manager->get_node_types_count();
}

void node_get_all_node_types(mush::node::type_storage * output, uint32_t count) {
	auto types = _manager->get_node_types();
	if (types.size() >= count && types.size() > 0) {
		memcpy(output, &types[0], count * sizeof(mush::node::type_storage));
	}
}

uint32_t node_get_linked_node_count(uint32_t node_id) {
	if_node_init();
	auto links = _manager->get_linked_nodes(node_id);
	return links.size();
}

bool node_get_linked_nodes(uint32_t node_id, uint32_t * output, uint32_t count) {
	if_node_init();
	auto links = _manager->get_linked_nodes(node_id);
	if (links.size() >= count && links.size() > 0) {
		memcpy(output, &links[0], count * sizeof(uint32_t));
		return true;
	} else {
		return false;
	}
}


void node_init() {
	_manager = std::make_shared<mush::node::manager>();
}

uint32_t node_create_processor() {
	if_node_init();
	if (_manager == nullptr) {
		node_init();
	}

	return _manager->create_processor();
}

uint32_t node_add_new_node(const uint32_t node_type_id) {
	if_node_init();
	uint32_t node_id = _manager->create_default_node(node_type_id);
	return node_id;
}

void node_add_node_to_processor(const uint32_t processor_id, const uint32_t node_id) {
	if_node_init();
	auto pro = _manager->get_processor(processor_id);
	if (pro != nullptr) {
		pro->add_node(node_id);
	}
}

uint32_t node_add_new_node_to_processor(const uint32_t processor_id, const uint32_t node_type_id) {
	if_node_init();
	uint32_t node_id = node_add_new_node(node_type_id);

	node_add_node_to_processor(processor_id, node_id);
    
    return node_id;
}

void node_add_node_to_link(const uint32_t node_id, const uint32_t node_id_to_link) {
	if_node_init();
	_manager->add_node_link(node_id, node_id_to_link);
}

mush::node::manager * node_get_node_manager() {
	if_node_init();
	return _manager.get();
}

uint32_t node_register_external_node_type(const char * name) {
	if_node_init();
	return _manager->register_node_type(name);
}

uint32_t node_register_external_node(std::shared_ptr<mush::processNode> node, uint32_t node_type_id) {
	if_node_init();
	return _manager->add_external_node(node, node_type_id);
}

void node_get_node_name(uint32_t node_id, char * output, uint32_t count) {
	auto name = _manager->get_node_name(node_id);
	if (NODE_NAME_LENGTH < count) {
		count = NODE_NAME_LENGTH;
	}
	memcpy(output, name, count * sizeof(char));
}

std::shared_ptr<mush::opencl> _context = nullptr;
std::shared_ptr<mush::gui::fake_window> _fake_window = nullptr;
std::shared_ptr<tagInGui> _tag_gui = nullptr;
std::mutex _gui_mutex;
std::deque<uint32_t> _gui_nodes_to_add;

const char * _node_get_resource_path() {
    
    const char * resourceDir = ".";
    
#ifdef __APPLE__
    NSString * frDir = [NSString stringWithFormat:@"%@/%@", [[NSBundle mainBundle] bundlePath], @"Contents/Frameworks/Video Mush.framework"];
    NSString * resDir = [[[NSBundle bundleWithPath:frDir] resourcePath] stringByAppendingString:@"/"];
    resourceDir = [resDir UTF8String];
#endif
    
    return resourceDir;
}

void if_context_init() {
	if (_fake_window == nullptr) {
		_fake_window = std::make_shared<mush::gui::fake_window>();
		_fake_window->acquire_context();
	}

    
	if (_context == nullptr) {
		_context = std::make_shared<mush::opencl>(_node_get_resource_path(), false);
		_context->init(true);
		_fake_window->release_context();
	}
}

std::vector<std::pair<int, int>> get_pairs_from_vector(std::vector<int> candidates, const int N) {
	std::unordered_map<int, bool> searchable;
	std::vector<std::pair<int, int>> pairs;
	int half_values = 0;

	for (auto m : candidates) {
		searchable.insert({ N - m, true });
		if (m * 2 == N) {
			half_values++;
		}
	}

	// Handling the half value of an even N as a special case
	if (half_values == 1) {
		auto half = searchable.find(N / 2);
		if (half != searchable.end()) {
			half->second = false;
		}
	}

	for (auto m : candidates) {
		auto s = searchable.find(m);
		if (s != searchable.end() && s->second == true) {
			pairs.push_back({ N - m, s->first });

			// Cleaning up; take both of these numbers out of contention
			s->second = false;
			auto s2 = searchable.find(N - m);
			if (s2 != searchable.end()) {
				s2->second = false;
			}
		}
	}

	return pairs;
}


void node_run_inits() {
	if_node_init();

	if_context_init();

	auto processors = _manager->get_all_processors();
	if (processors.size() > 0) {
		for (auto& p : processors) {
			_manager->get_processor(p)->init(_context, {});
		}
	}

	std::vector<int> test = { 3, 2, 4, 6, 8, 7, -1, -2, 2, 2, 2, 4, 4 };
	int t = 6;

	auto v = get_pairs_from_vector(test, t);

	putLog("First run, with repeats");
	for (auto vv : v) {
		std::stringstream strm;
		strm << vv.first << " " << vv.second;
		putLog(strm.str());
	}
	putLog("Second run, without repeats");
	std::vector<int> test2 = { 3, 3, 3, 3, 2, 4, 6, 8, 7, -1, -2, 2, 2, 2, 4, 4 };
	auto v2 = get_pairs_from_vector(test2, t);

	for (auto vv : v2) {
		std::stringstream strm;
		strm << vv.first << " " << vv.second;
		putLog(strm.str());
	}
}

void node_create_gui() {
	if_node_init();

	if_context_init();

	//if (_fake_window == nullptr) {
	_fake_window = std::make_shared<mush::gui::fake_window>();
	_fake_window->acquire_context();
	//}

	_tag_gui = std::make_shared<tagInGui>();

	_tag_gui->createGLContext(false, false, _node_get_resource_path());
	_tag_gui->setContext(_context);
	_tag_gui->postContextSetup();

	std::vector<std::shared_ptr<mush::guiAccessible>> guis = {};
    std::string exr_path = std::string(_node_get_resource_path()) + "/EXR";
	_tag_gui->init(_context, guis, exr_path.c_str(), 8);
	_tag_gui->update();
}

void node_update_gui() {
	if (_tag_gui != nullptr) {
		std::lock_guard<std::mutex> lock(_gui_mutex);

		while (_gui_nodes_to_add.size() > 0) {
			auto i = _gui_nodes_to_add.front();
			_gui_nodes_to_add.pop_front();

			auto node = _manager->get_cast_node<mush::guiAccessible>(i);
			if (node != nullptr) {
				_tag_gui->addSubScreen(node);
			}

		}
		_tag_gui->update();
	}
}

bool node_add_to_gui(uint32_t node_id) {
	std::lock_guard<std::mutex> lock(_gui_mutex);
	_gui_nodes_to_add.push_back(node_id);
	return false;
}

void node_process(uint32_t node_id) {
	if_node_init();
	auto node = _manager->get_process_node(node_id);
	if (node != nullptr) {
		node->process();
	}
}

bool node_is_inited(uint32_t node_id) {
	if_node_init();
	auto node = _manager->get_ring_node(node_id);
	if (node != nullptr) {
		if (node->getBufferCount() > 0) {
			return true;
		}
	}
	return false;
}
