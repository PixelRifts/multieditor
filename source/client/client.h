/* date = August 17th 2022 6:11 pm */

#ifndef CLIENT_H
#define CLIENT_H

typedef string_array PluginAssociatedStringsGet(M_Arena* arena);
typedef void PluginInitProcedure(string s);
typedef void PluginUpdateProcedure(f32 dt);
typedef void PluginKeyProcedure(u8 key, i32 action);
typedef void PluginButtonProcedure(u8 button, i32 action);
typedef void PluginResizeProcedure(i32 w, i32 h);
typedef void PluginCustomRenderProcedure(void);
typedef void PluginRenderProcedure(R2D_Renderer* renderer);
typedef void PluginFreeProcedure(void);

typedef struct C_Plugin {
    OS_Library lib;
    PluginInitProcedure* init;
    PluginUpdateProcedure* update;
    PluginCustomRenderProcedure* custom_render;
    PluginRenderProcedure* render;
    PluginFreeProcedure* free;
	
	PluginKeyProcedure* key;
	PluginButtonProcedure* button;
	PluginResizeProcedure* resize;
	
	string_array associated_extensions;
} C_Plugin;


#endif //CLIENT_H
