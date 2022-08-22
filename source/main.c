#include "defines.h"
#include "os/os.h"
#include "os/window.h"
#include "os/input.h"
#include "base/tctx.h"
#include "base/utils.h"
#include "core/backend.h"
#include "core/resources.h"
#include "opt/render_2d.h"
#include "opt/ui.h"
#include "client/client.h"
#include "client/fexp.h"

typedef void init_func(void);
typedef void render_func(R2D_Renderer* renderer);
typedef void free_func(void);

//~
// Main Codebase Additions: 
// [ ] Input_Key_Period and Input_Key_Comma are swapped
// [ ] Blending in pipelines
// [ ] Utils FixFilepath Line 64 Bug
// [ ] Callbacks in windows
// [ ] R2D_Resize
// [ ] Maximize resize event
// [ ] Button Up events change BUTTONDOWN -> BUTTONUP
//
//~

Array_Prototype(plugin_array, C_Plugin);
Array_Impl(plugin_array, C_Plugin);
Array_Prototype(string_array_array, string_array);
Array_Impl(string_array_array, string_array);

static plugin_array all_plugins = {0};
static string_array_array plugin_associated_names = {0};
static fexp_context explorer_context = {0};
static i32 plugin_idx = -1;
static M_Arena global_arena;
static R2D_Renderer renderer;

void FilloutPluginStructures(M_Arena* arena) {
	M_Scratch scratch = scratch_get();
    OS_FileIterator iterator = OS_FileIterInit(str_lit("plugins"));
    string name; OS_FileProperties props;
    while (OS_FileIterNext(&scratch.arena, &iterator, &name, &props)) {
        u64 last_dot = str_find_last(name, str_lit("."), 0);
        string ext = { name.str + last_dot, name.size - last_dot };
        if (str_eq(ext, str_lit("dll"))) {
            OS_Library lib = OS_LibraryLoad(str_cat(&scratch.arena, str_lit("plugins/"), name));
            
            PluginInitProcedure* init_proc =
			(PluginInitProcedure*) OS_LibraryGetFunction(lib, "Init");
            PluginUpdateProcedure* update_proc = (PluginUpdateProcedure*) OS_LibraryGetFunction(lib, "Update");
            PluginCustomRenderProcedure* custom_render_proc = (PluginCustomRenderProcedure*) OS_LibraryGetFunction(lib, "CustomRender");
            PluginRenderProcedure* render_proc = (PluginRenderProcedure*) OS_LibraryGetFunction(lib, "Render");
            PluginFreeProcedure* free_proc =
			(PluginFreeProcedure*) OS_LibraryGetFunction(lib, "Free");
            
			PluginKeyProcedure* key_proc =
			(PluginKeyProcedure*) OS_LibraryGetFunction(lib, "OnKey");
            PluginButtonProcedure* button_proc =
			(PluginButtonProcedure*) OS_LibraryGetFunction(lib, "OnButton");
            PluginResizeProcedure* resize_proc =
			(PluginResizeProcedure*) OS_LibraryGetFunction(lib, "OnResize");
            
			PluginAssociatedStringsGet* associate = (PluginAssociatedStringsGet*) OS_LibraryGetFunction(lib, "Extensions");
			
            C_Plugin plugin = {
				.lib = lib,
				.init = init_proc,
				.update = update_proc,
				.custom_render = custom_render_proc,
				.render = render_proc,
				.free = free_proc,
				.key = key_proc,
				.button = button_proc,
				.resize = resize_proc,
			};
            if (associate) {
				string_array strings = associate(arena);
				string_array_array_add(&plugin_associated_names, strings);
			} else {
				string_array_array_add(&plugin_associated_names, (string_array) {0});
			}
            plugin_array_add(&all_plugins, plugin);
        }
    }
    OS_FileIterEnd(&iterator);
    
    scratch_return(&scratch);
}

void key_callback(OS_Window* window, u8 key, i32 action) {
	if (plugin_idx == -1) {
		fexp_input_key(&explorer_context, window, key, action);
	} else {
		if (action == Input_Press) {
			if (key == Input_Key_LeftArrow && OS_InputKey(Input_Key_Alt)) {
				if (all_plugins.elems[plugin_idx].free)
					all_plugins.elems[plugin_idx].free();
				
				plugin_idx = -1;
			}
		} else {
			if (all_plugins.elems[plugin_idx].key)
				all_plugins.elems[plugin_idx].key(key, action);
		}
	}
}

void button_callback(OS_Window* window, u8 button, i32 action) {
	if (plugin_idx != -1) {
		if (all_plugins.elems[plugin_idx].button)
			all_plugins.elems[plugin_idx].button(button, action);
	}
}

void resize_callback(OS_Window* window, i32 w, i32 h) {
	if (!w || !h) return;
	R_Viewport(0, 0, w, h);
	R2D_ResizeProjection(&renderer, (vec2) { (f32)w, (f32)h });
	
	if (plugin_idx != -1) {
		if (all_plugins.elems[plugin_idx].resize)
			all_plugins.elems[plugin_idx].resize(w, h);
	}
}

b8 check_plugin(string name) {
	string name_ext = U_GetExtensionFromFilepath(name);
	Iterate(plugin_associated_names, i) {
		string_array sarr = plugin_associated_names.elems[i];
		Iterate(sarr, k) {
			string curr = sarr.elems[k];
			if (str_eq(curr, name_ext)) {
				plugin_idx = i;
				string raised = str_copy(&global_arena, name);
				if (all_plugins.elems[plugin_idx].init)
					all_plugins.elems[plugin_idx].init(raised);
				return true;
			}
		}
	}
	return false;
}

int main() {
	OS_Init();
	
	global_arena = (M_Arena) {0};
	arena_init(&global_arena);
	
	ThreadContext context = {0};
	tctx_init(&context);
	
	FilloutPluginStructures(&global_arena);
	
	OS_Window* window = OS_WindowCreate(1080, 720, str_lit("This should work"));
	window->key_callback = key_callback;
	window->resize_callback = resize_callback;
	B_BackendInit(window);
	OS_WindowShow(window);
	
	renderer = (R2D_Renderer) {0};
	R2D_Init((vec2) { window->width, window->height }, &renderer);
	UI_Init(&renderer);
	
	R2D_FontInfo font;
	R2D_FontLoad(&font, str_lit("res/Inconsolata.ttf"), 22);
	explorer_context.font = &font;
	fexp_init(&explorer_context);
	
	f32 start, end, dt;
	start = OS_TimeMicrosecondsNow();
	
	while (OS_WindowIsOpen(window)) {
		OS_PollEvents();
		end = OS_TimeMicrosecondsNow();
		dt = (end - start) / 1e6;
		start = OS_TimeMicrosecondsNow();
		
		if (plugin_idx == -1) {
			fexp_update(&explorer_context, dt);
		} else {
			if (all_plugins.elems[plugin_idx].update)
				all_plugins.elems[plugin_idx].update(dt);
		}
		
		R_Clear(BufferMask_Color);
		
		if (plugin_idx != -1) {
			if (all_plugins.elems[plugin_idx].custom_render)
				all_plugins.elems[plugin_idx].custom_render();
		}
		
		R2D_BeginDraw(&renderer);
		if (plugin_idx == -1) {
			fexp_render(&explorer_context, &renderer);
		} else {
			if (all_plugins.elems[plugin_idx].render)
				all_plugins.elems[plugin_idx].render(&renderer);
		}
		R2D_EndDraw(&renderer);
		
		B_BackendSwapchainNext(window);
	}
	
	if (plugin_idx != -1) {
		if (all_plugins.elems[plugin_idx].free)
			all_plugins.elems[plugin_idx].free();
	}
	
	UI_Free();
	R2D_FontFree(&font);
	R2D_Free(&renderer);
	
	B_BackendFree(window);
	
	OS_WindowClose(window);
	
	tctx_free(&context);
	
	arena_free(&global_arena);
}
