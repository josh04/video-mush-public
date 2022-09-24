//
//  node_api.hpp
//  mush-core
//
//  Created by Josh McNamee on 17/07/2017.
//  Copyright © 2017 josh04. All rights reserved.
//

#ifndef node_api_hpp
#define node_api_hpp

#include <memory>
#include <stdint.h>
#include "mush-core-dll.hpp"

#ifdef _WIN32
#define MUSH_DX
#endif

namespace mush {
	class processNode;
	class initNode;
	namespace node {
		class manager;
		struct type_storage;
		struct info;
	}
}

extern "C" {
	//void node_init();

	MUSHEXPORTS_API uint32_t node_get_total_node_count();
	MUSHEXPORTS_API uint32_t node_get_total_processor_count();
	MUSHEXPORTS_API uint32_t node_get_processor_node_count(uint32_t processor_id);
	MUSHEXPORTS_API void node_get_all_processors(uint32_t * output, uint32_t count);
	MUSHEXPORTS_API void node_get_all_nodes(mush::node::info * output, uint32_t count);
	MUSHEXPORTS_API bool node_get_all_nodes_from_processor(uint32_t processor_id, uint32_t * output, uint32_t count);
	MUSHEXPORTS_API uint32_t node_get_node_type_count();
	MUSHEXPORTS_API void node_get_all_node_types(mush::node::type_storage * output, uint32_t count);

	MUSHEXPORTS_API uint32_t node_get_linked_node_count(uint32_t node_id);
	MUSHEXPORTS_API bool node_get_linked_nodes(uint32_t node_id, uint32_t * output, uint32_t count);

	MUSHEXPORTS_API uint32_t node_create_processor();
	MUSHEXPORTS_API uint32_t node_add_new_node(const uint32_t node_type_id);
	MUSHEXPORTS_API void node_add_node_to_processor(const uint32_t processor_id, const uint32_t node_id);
	MUSHEXPORTS_API uint32_t node_add_new_node_to_processor(const uint32_t processor_id, const uint32_t node_type_id);
	MUSHEXPORTS_API void node_add_node_to_link(const uint32_t node_id, const uint32_t node_id_to_link);

	MUSHEXPORTS_API uint32_t node_register_external_node_type(const char * name);
	MUSHEXPORTS_API uint32_t node_register_external_node(std::shared_ptr<mush::processNode> node, const uint32_t node_type_id);
	MUSHEXPORTS_API void node_get_node_name(uint32_t node_id, char * output, uint32_t count);

	MUSHEXPORTS_API void node_run_inits();
	MUSHEXPORTS_API void node_create_gui();
	MUSHEXPORTS_API void node_update_gui();

	MUSHEXPORTS_API bool node_add_to_gui(uint32_t node_id);

	MUSHEXPORTS_API void node_process(uint32_t node_id);

	MUSHEXPORTS_API mush::node::manager * node_get_node_manager();
	MUSHEXPORTS_API bool node_is_inited(uint32_t node_id);
}


#endif /* node_api_hpp */
