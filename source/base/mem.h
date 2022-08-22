/* date = September 27th 2021 11:45 am */

#ifndef MEM_H
#define MEM_H

#include <stdlib.h>
#include "defines.h"

//~ Arena (Linear Allocator)

typedef struct M_Arena {
    u8* memory;
    u64 max;
    u64 alloc_position;
    u64 commit_position;
    b8 static_size;
} M_Arena;

#define M_ARENA_MAX Gigabytes(1)
#define M_ARENA_COMMIT_SIZE Kilobytes(8)

dll_plugin_api void* arena_alloc(M_Arena* arena, u64 size);
dll_plugin_api void* arena_alloc_zero(M_Arena* arena, u64 size);
dll_plugin_api void  arena_dealloc(M_Arena* arena, u64 size);
dll_plugin_api void  arena_dealloc_to(M_Arena* arena, u64 pos);
dll_plugin_api void* arena_raise(M_Arena* arena, void* ptr, u64 size);
dll_plugin_api void* arena_alloc_array_sized(M_Arena* arena, u64 elem_size, u64 count);

#define arena_alloc_array(arena, elem_type, count) \
arena_alloc_array_sized(arena, sizeof(elem_type), count)

dll_plugin_api void arena_init(M_Arena* arena);
dll_plugin_api void arena_init_sized(M_Arena* arena, u64 max);
dll_plugin_api void arena_clear(M_Arena* arena);
dll_plugin_api void arena_free(M_Arena* arena);

typedef struct M_ArenaTemp {
    M_Arena* arena;
    u64 pos;
} M_ArenaTemp;

dll_plugin_api M_ArenaTemp arena_begin_temp(M_Arena* arena);
dll_plugin_api void        arena_end_temp(M_ArenaTemp temp);

//~ Scratch Helpers
// A scratch block is just a view into an arena
#include "tctx.h"

dll_plugin_api M_Scratch scratch_get(void);
dll_plugin_api void scratch_reset(M_Scratch* scratch);
dll_plugin_api void scratch_return(M_Scratch* scratch);

#endif //MEM_H
