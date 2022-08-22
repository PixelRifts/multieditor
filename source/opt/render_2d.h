/* date = July 20th 2022 9:08 pm */

#ifndef RENDER_2D_H
#define RENDER_2D_H

#include "defines.h"
#include "base/base.h"
#include "core/resources.h"

#include <stb/stb_truetype.h>

//~ Fonts

typedef struct R2D_FontInfo {
    R_Texture2D font_texture;
    stbtt_packedchar cdata[95];
    f32 scale;
    f32 font_size;
    i32 ascent;
    i32 descent;
    i32 baseline;
} R2D_FontInfo;

dll_plugin_api void R2D_FontLoad(R2D_FontInfo* fontinfo, string filename, f32 size);
dll_plugin_api void R2D_FontFree(R2D_FontInfo* fontinfo);

//~ Render Internals

typedef struct R2D_Vertex {
    vec2 pos;
    vec2 tex_coords;
    f32  tex_index;
    vec4 color;
	vec3 roundingparams;
	vec2 vertid;
} R2D_Vertex;

#define R2D_MAX_INTERNAL_CACHE_VCOUNT 1024

typedef struct R2D_VertexCache {
    R2D_Vertex* vertices;
    u32 count;
    u32 max_verts;
} R2D_VertexCache;

dll_plugin_api R2D_VertexCache R2D_VertexCacheCreate(M_Arena* arena, u32 max_verts);
dll_plugin_api void R2D_VertexCacheReset(R2D_VertexCache* cache);
dll_plugin_api b8   R2D_VertexCachePush(R2D_VertexCache* cache, R2D_Vertex* vertices, u32 vertex_count);

typedef struct R2D_Batch {
	R2D_VertexCache cache;
    R_Texture2D *textures[8];
    u8 tex_count;
} R2D_Batch;

Array_Prototype(R2D_BatchArray, R2D_Batch);

//~ Render API

typedef struct R2D_Renderer {
	M_Arena arena;
	
	R2D_BatchArray batches;
    u8 current_batch;
    rect cull_quad;
    vec2 offset;
    
	R_Texture2D white_texture;
	
	R_Pipeline pipeline;
	R_Buffer buffer;
	R_ShaderPack shader;
} R2D_Renderer;

dll_plugin_api void R2D_Init(vec2 render_size, R2D_Renderer* renderer);
dll_plugin_api void R2D_Free(R2D_Renderer* renderer);
dll_plugin_api void R2D_ResizeProjection(R2D_Renderer* renderer, vec2 render_size);

dll_plugin_api void R2D_BeginDraw(R2D_Renderer* renderer);
dll_plugin_api void R2D_EndDraw(R2D_Renderer* renderer);

dll_plugin_api rect R2D_PushCullRect(R2D_Renderer* renderer, rect new_quad);
dll_plugin_api void R2D_PopCullRect(R2D_Renderer* renderer, rect old_quad);
dll_plugin_api vec2 R2D_PushOffset(R2D_Renderer* renderer, vec2 new_offset);
dll_plugin_api void R2D_PopOffset(R2D_Renderer* renderer, vec2 old_offset);

dll_plugin_api void R2D_DrawQuad(R2D_Renderer* renderer, rect quad, R_Texture2D* texture, rect uvs, vec4 color, f32 rounding);
dll_plugin_api void R2D_DrawQuadC(R2D_Renderer* renderer, rect quad, vec4 color, f32 rounding);
dll_plugin_api void R2D_DrawQuadT(R2D_Renderer* renderer, rect quad, R_Texture2D* texture, vec4 tint, f32 rounding);
dll_plugin_api void R2D_DrawQuadST(R2D_Renderer* renderer, rect quad, R_Texture2D* texture, rect uvs, vec4 tint, f32 rounding);

dll_plugin_api void R2D_DrawString(R2D_Renderer* renderer, R2D_FontInfo* fontinfo, vec2 pos, string str);
dll_plugin_api void R2D_DrawStringC(R2D_Renderer* renderer, R2D_FontInfo* fontinfo, vec2 pos, string str, vec4 color);
dll_plugin_api f32 R2D_GetStringSize(R2D_FontInfo* fontinfo, string str);

#endif //RENDER_2D_H
