//
//  video_node_api.hpp
//  video-mush
//
//  Created by Josh McNamee on 23/07/2017.
//
//

#ifndef video_node_api_hpp
#define video_node_api_hpp

#include <stdint.h>
#include <Mush Core/inputConfig.hpp>
#include "dll.hpp"

#include <Mush Core/node_api.hpp>

#ifdef MUSH_DX
#include <WinDef.h>
#endif

extern "C" {

	struct vnode_add_new_input_return {
		uint32_t processor_id;
		uint32_t i_id;
		uint32_t pre_id;
	};

	VIDEOMUSH_EXPORTS vnode_add_new_input_return vnode_add_new_input(mush::core::inputConfigStruct input_config);
	VIDEOMUSH_EXPORTS bool vnode_spawn_input_thread(uint32_t i_id);
    
#ifdef MUSH_DX
	VIDEOMUSH_EXPORTS void vnode_create_dx_context(HWND hwnd);
	VIDEOMUSH_EXPORTS bool vnode_has_dx_context();
    VIDEOMUSH_EXPORTS void * vnode_get_dx_texture();
#endif

	VIDEOMUSH_EXPORTS const char * vnode_get_name(uint32_t node_type_id);
}

#endif /* video_node_api_hpp */
