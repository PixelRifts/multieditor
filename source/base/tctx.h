/* date = June 30th 2022 0:24 pm */

#ifndef TCTX_H
#define TCTX_H

#include "defines.h"

#define M_SCRATCH_SIZE Kilobytes(32)

typedef struct M_Scratch {
    M_Arena arena;
    u32 index;
} M_Scratch;

typedef struct scratch_free_list_node scratch_free_list_node;
struct scratch_free_list_node {
    scratch_free_list_node* next;
    u32 index;
};

typedef struct ThreadContext {
	M_Arena arena;
    u32 max_created;
    scratch_free_list_node* free_list;
} ThreadContext;

dll_plugin_api void tctx_init(ThreadContext* ctx);
dll_plugin_api void tctx_free(ThreadContext* ctx);

dll_plugin_api M_Scratch tctx_scratch_get(ThreadContext* ctx);
dll_plugin_api void tctx_scratch_reset(ThreadContext* ctx, M_Scratch* scratch);
dll_plugin_api void tctx_scratch_return(ThreadContext* ctx, M_Scratch* scratch);

#endif //TCTX_H
