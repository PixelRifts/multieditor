/* date = August 17th 2022 6:32 pm */

#ifndef FEXP_H
#define FEXP_H

#include "defines.h"
#include "base/str.h"
#include "base/vmath.h"
#include "base/utils.h"
#include "opt/render_2d.h"
#include "os/os.h"
#include "os/input.h"

typedef u32 InputMode;
enum {
	InputMode_Regular,
	InputMode_Drive,
};

typedef struct fexp_context {
    M_Arena arena;
    R2D_FontInfo* font;
	
    i32 selected_index;
    u32 latest_count;
    rect selection_rect;
    rect target_selection_rect;
    string current_filepath;
	
	InputMode mode;
    string current_query;
	string stored_query;
    i32 current_query_idx;
	i32 stored_query_idx;
    
	b8 inited;
	b8 swapped_to_other_mode;
} fexp_context;

void fexp_init(fexp_context* ctx);
void fexp_update(fexp_context* ctx, f32 dt);
void fexp_input_key(fexp_context* ctx, OS_Window* window, u8 key, i32 action);
void fexp_render(fexp_context* ctx, R2D_Renderer* cb);

#endif //FEXP_H
