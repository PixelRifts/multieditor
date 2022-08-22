#include "fexp.h"

b8 check_plugin(string name);

void fexp_init(fexp_context* ctx) {
    arena_init(&ctx->arena);
    ctx->current_filepath = OS_Filepath(&ctx->arena, SystemPath_CurrentDir);
    ctx->current_query = str_alloc(&ctx->arena, PATH_MAX);
    ctx->stored_query = str_alloc(&ctx->arena, PATH_MAX);
    ctx->inited = false;
    ctx->current_query_idx = 0;
}

void fexp_update(fexp_context* ctx, f32 dt) {
    //- Updating 
	M_Scratch scratch = scratch_get();
    
	ctx->latest_count = 0;
    animate_f32exp(&ctx->selection_rect.x, ctx->target_selection_rect.x, 40.f, dt);
    animate_f32exp(&ctx->selection_rect.y, ctx->target_selection_rect.y, 40.f, dt);
    animate_f32exp(&ctx->selection_rect.w, ctx->target_selection_rect.w, 40.f, dt);
    animate_f32exp(&ctx->selection_rect.h, ctx->target_selection_rect.h, 40.f, dt);
    
    string fixed = str_cat(&scratch.arena, ctx->current_filepath, str_lit("/*"));
    string fixed_query = { .str = ctx->current_query.str, .size = ctx->current_query_idx };
    
    OS_FileIterator iter = OS_FileIterInitPattern(fixed);
    string name; OS_FileProperties props;
    while (OS_FileIterNext(&scratch.arena, &iter, &name, &props)) {
        if (str_find_first(name, fixed_query, 0) != name.size) {
            ctx->latest_count++;
        }
    }
    OS_FileIterEnd(&iter);
    
	i32 prev_selected_index = ctx->selected_index;
    
    if (OS_InputKeyPressed(Input_Key_DownArrow) || OS_InputKeyHeld(Input_Key_DownArrow)) {
        ctx->selected_index++;
    } else if (OS_InputKeyPressed(Input_Key_UpArrow) || OS_InputKeyHeld(Input_Key_UpArrow)) {
        ctx->selected_index--;
    }
    
    ctx->selected_index = Wrap(0, ctx->selected_index, ctx->latest_count - 1);
    
    b8 folder_changed = ctx->swapped_to_other_mode;
    
	iter = OS_FileIterInitPattern(fixed);
    string selection_name = {0};
    u32 k = 0;
    while (OS_FileIterNext(&scratch.arena, &iter, &name, &props)) {
        if (str_find_first(name, fixed_query, 0) != name.size) {
            
            if (k == ctx->selected_index) {
                selection_name = name;
                if (OS_InputKeyPressed(Input_Key_Enter) && !ctx->swapped_to_other_mode) {
                    // Open File/Folder
                    if (props.flags & FileProperty_IsFolder) {
                        ctx->current_filepath = str_cat(&scratch.arena, ctx->current_filepath, str_lit("/"));
                        ctx->current_filepath = str_cat(&scratch.arena, ctx->current_filepath, name);
                        ctx->current_filepath = U_FixFilepath(&ctx->arena, ctx->current_filepath);
                    } else {
						string tmp = str_cat(&scratch.arena, ctx->current_filepath, str_lit("/"));
						tmp = str_cat(&scratch.arena, tmp, name);
                        
						if (!check_plugin(tmp)) {
							OS_FileOpen(name);
						}
					}
					
                    folder_changed = true;
                    break;
                }
            }
            k++;
            
        }
    }
    OS_FileIterEnd(&iter);
    
    if ((OS_InputKey(Input_Key_Control)) && OS_InputKeyPressed(Input_Key_Backspace)) {
		u32 khi = str_find_first(ctx->current_filepath, str_lit("/"), 0);
		if (khi != ctx->current_filepath.size) {
			ctx->current_filepath = str_cat(&scratch.arena, ctx->current_filepath, str_lit("/.."));
			ctx->current_filepath = U_FixFilepath(&ctx->arena, ctx->current_filepath);
			folder_changed = true;
		} else {
			ctx->current_filepath = str_lit("");
		}
	}
	
	if (folder_changed) {
		ctx->selected_index = 0;
		ctx->current_query_idx = 0;
	}
	
	if (prev_selected_index != ctx->selected_index || !ctx->inited) {
		f32 size = 0.f;
		if (selection_name.size != 0) {
			size = R2D_GetStringSize(ctx->font, selection_name);
		}
		f32 y = ctx->font->font_size * 2.5 + (ctx->font->font_size + 2) * ctx->selected_index;
		ctx->target_selection_rect = (rect) { 10, y - 16, 8 + size, 22 };
		ctx->inited = true;
	}
	
	if (folder_changed) {
		ctx->inited = false;
	}
	ctx->swapped_to_other_mode = false;
	
	scratch_return(&scratch);
}

void fexp_input_key(fexp_context* ctx, OS_Window* window, u8 key, i32 action) {
    if (action == Input_Press) {
		if (ctx->mode == InputMode_Drive) {
			if (key == Input_Key_Enter) {
				string swap = ctx->stored_query;
				ctx->stored_query = ctx->current_query;
				ctx->stored_query_idx = ctx->current_query_idx;
				ctx->current_query = swap;
				ctx->current_query_idx = 0;
				ctx->current_filepath = ctx->stored_query;
				ctx->current_filepath.size = ctx->stored_query_idx;
				ctx->mode = InputMode_Regular;
				ctx->swapped_to_other_mode = true;
				return;
			} else if (key == Input_Key_Escape) {
				string swap = ctx->stored_query;
				i32 swap_idx = ctx->stored_query_idx;
				ctx->stored_query = ctx->current_query;
				ctx->stored_query_idx = ctx->current_query_idx;
				ctx->current_query = swap;
				ctx->current_query_idx = swap_idx;
				ctx->mode = InputMode_Regular;
				ctx->swapped_to_other_mode = true;
				return;
			}
		}
		
		if ((key >= 'A' && key <= 'Z')) {
			if (ctx->mode == InputMode_Regular) {
				if (key == 'D' && OS_InputKey(Input_Key_Control)) {
					ctx->mode = InputMode_Drive;
					string swap = ctx->stored_query;
					i32 swap_idx = ctx->stored_query_idx;
					ctx->stored_query = ctx->current_query;
					ctx->stored_query_idx = ctx->current_query_idx;
					ctx->current_query = swap;
					ctx->current_query_idx = swap_idx;
					ctx->swapped_to_other_mode = false;
					return;
				}
			}
			
			u8 c = 65;
			if (OS_InputKey(Input_Key_Shift)) {
				c = (u8) key;
			} else
				c = (u8) key + 32;
			
			ctx->current_query.str[ctx->current_query_idx++] = c;
			ctx->inited = false;
		} else if ((
					key == Input_Key_Period ||
					key == Input_Key_Minus ||
					key == Input_Key_Apostrophe
					) && !(OS_InputKey(Input_Key_Shift))) {
			u8 c = key;
			ctx->current_query.str[ctx->current_query_idx++] = c;
			
			ctx->inited = false;
		} else if ((key == Input_Key_Minus) && (OS_InputKey(Input_Key_Shift))) {
			u8 c = 95; // ascii _
			ctx->current_query.str[ctx->current_query_idx++] = c;
			
			ctx->inited = false;
		} else if (key == Input_Key_Backspace && !(OS_InputKey(Input_Key_Control))) {
			ctx->current_query_idx--;
			ctx->current_query_idx = Max(0, ctx->current_query_idx);
			
			ctx->inited = false;
		} else if (key == Input_Key_Semicolon) {
			if (OS_InputKey(Input_Key_Shift)) {
				ctx->current_query.str[ctx->current_query_idx++] = ':';
			} else ctx->current_query.str[ctx->current_query_idx++] = ';';
		}
	}
}


void fexp_render(fexp_context* ctx, R2D_Renderer* cb) {
	M_Scratch scratch = scratch_get();
	string fixed_to_render = str_cat(&scratch.arena, ctx->current_filepath, str_lit("/"));
	string fixed = str_cat(&scratch.arena, ctx->current_filepath, str_lit("/*"));
	string fixed_query = { .str = ctx->current_query.str, .size = ctx->current_query_idx };
	string fixed_full_query = str_cat(&scratch.arena, fixed_to_render, fixed_query);
	if (ctx->mode == InputMode_Drive) {
		fixed_full_query = (string) { .str = ctx->current_query.str, .size = ctx->current_query_idx };
		fixed_full_query = str_cat(&scratch.arena, str_lit("Enter Drive: "), fixed_full_query);
	}
	
	f32 y = ctx->font->font_size * 2.5;
	R2D_DrawStringC(cb, ctx->font, (vec2) { 16, ctx->font->font_size * 1.15f }, fixed_full_query, (vec4) { .3f, .4f, .8f, 1.f });
	R2D_DrawQuadC(cb, (rect) { 0, ctx->font->font_size * 1.55f, cb->cull_quad.w, 1.f }, (vec4) { .8f, .4, .3f, 2.f }, 1.f);
	
	if (ctx->mode == InputMode_Regular) {
		OS_FileIterator iter = OS_FileIterInitPattern(fixed);
		u32 idx = 0;
		string name; OS_FileProperties props;
		
		R2D_DrawQuadC(cb, ctx->selection_rect, (vec4) { .3f, .3f, .3f, 1.f }, 4.f);
		
		while (OS_FileIterNext(&scratch.arena, &iter, &name, &props)) {
			if (str_find_first(name, fixed_query, 0) != name.size) {
				R2D_DrawString(cb, ctx->font, (vec2) { 14, y }, name);
				y += ctx->font->font_size + 2;
				idx++;
			}
		}
		
		OS_FileIterEnd(&iter);
	}
	
	scratch_return(&scratch);
}
