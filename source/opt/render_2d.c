#include "render_2d.h"

//~ Font Loading

void R2D_FontLoad(R2D_FontInfo* fontinfo, string filename, f32 size) {
    FILE* ttfile = fopen((char*)filename.str, "rb");
    AssertTrue(ttfile, "Font file '%.*s' couldn't be opened", str_expand(filename));
    fseek(ttfile, 0, SEEK_END);
    u64 length = ftell(ttfile);
    rewind(ttfile);
    u8 buffer[length * sizeof(u8)];
    fread(buffer, length, 1, ttfile);
    fclose(ttfile);
    
    u8 temp_bitmap[512 * 512];
    
    stbtt_fontinfo finfo;
    stbtt_pack_context packctx;
    stbtt_InitFont(&finfo, buffer, 0);
    stbtt_PackBegin(&packctx, temp_bitmap, 512, 512, 0, 1, 0);
    stbtt_PackSetOversampling(&packctx, 1, 1);
    stbtt_PackFontRange(&packctx, buffer, 0, size, 32, 95, fontinfo->cdata);
    stbtt_PackEnd(&packctx);
    
	R_Texture2DAlloc(&fontinfo->font_texture, TextureFormat_R, 512, 512, TextureResize_Linear, TextureResize_Linear, TextureWrap_ClampToEdge, TextureWrap_ClampToEdge);
	i32 swizzles[4] = {
		TextureChannel_One, TextureChannel_One,
		TextureChannel_One, TextureChannel_R
	};
	R_Texture2DSwizzle(&fontinfo->font_texture, swizzles);
	R_Texture2DData(&fontinfo->font_texture, temp_bitmap);
	
	fontinfo->scale = stbtt_ScaleForPixelHeight(&finfo, size);
	stbtt_GetFontVMetrics(&finfo, &fontinfo->ascent, &fontinfo->descent, nullptr);
	fontinfo->baseline = (i32) (fontinfo->ascent * fontinfo->scale);
	fontinfo->font_size = size;
}

void R2D_FontFree(R2D_FontInfo* fontinfo) {
	R_Texture2DFree(&fontinfo->font_texture);
}

//~ Internals

Array_Impl(R2D_BatchArray, R2D_Batch);

static R2D_Batch* R2D_NextBatch(R2D_Renderer* renderer) {
    R2D_Batch* next = &renderer->batches.elems[++renderer->current_batch];
    
    if (renderer->current_batch >= renderer->batches.len) {
		R2D_BatchArray_add(&renderer->batches, (R2D_Batch) {});
		next = &renderer->batches.elems[renderer->current_batch];
        next->cache = R2D_VertexCacheCreate(&renderer->arena, R2D_MAX_INTERNAL_CACHE_VCOUNT);
    }
    return next;
}

static b8 R2D_BatchCanAddTexture(R2D_Renderer* renderer, R2D_Batch* batch, R_Texture2D* texture) {
    if (batch->tex_count < 8) return true;
    for (u8 i = 0; i < batch->tex_count; i++) {
        if (R_Texture2DEquals(batch->textures[i], texture))
            return true;
    }
    return false;
}

static u8 R2D_BatchAddTexture(R2D_Renderer* renderer, R2D_Batch* batch, R_Texture2D* tex) {
    for (u8 i = 0; i < batch->tex_count; i++) {
		if (R_Texture2DEquals(batch->textures[i], tex))
            return i;
    }
    batch->textures[batch->tex_count] = tex;
    return batch->tex_count++;
}

static R2D_Batch* R2D_BatchGetCurrent(R2D_Renderer* renderer, int num_verts, R_Texture2D* tex) {
    R2D_Batch* batch = &renderer->batches.elems[renderer->current_batch];
    if (!R2D_BatchCanAddTexture(renderer, batch, tex) || batch->cache.count + num_verts >= batch->cache.max_verts)
        batch = R2D_NextBatch(renderer);
    return batch;
}

//~ Vertex Cache

R2D_VertexCache R2D_VertexCacheCreate(M_Arena* arena, u32 max_verts) {
	return (R2D_VertexCache) {
        .vertices = arena_alloc(arena, sizeof(R2D_Vertex) * max_verts),
        .count = 0,
        .max_verts = max_verts
    };
}

void R2D_VertexCacheReset(R2D_VertexCache* cache) {
	cache->count = 0;
}

b8 R2D_VertexCachePush(R2D_VertexCache* cache, R2D_Vertex* vertices, u32 vertex_count) {
	if (cache->max_verts < cache->count + vertex_count)
        return false;
    memcpy(cache->vertices + cache->count, vertices, sizeof(R2D_Vertex) * vertex_count);
    cache->count += vertex_count;
    return true;
}

//~ Renderer Core

void R2D_Init(vec2 render_size, R2D_Renderer* renderer) {
	arena_init(&renderer->arena);
	
	renderer->current_batch = 0;
	renderer->cull_quad = (rect) { 0, 0, render_size.x, render_size.y };
    renderer->offset = (vec2) { 0.f, 0.f };
	R2D_BatchArray_add(&renderer->batches, (R2D_Batch) {0});
	renderer->batches.elems[renderer->current_batch].cache = R2D_VertexCacheCreate(&renderer->arena, R2D_MAX_INTERNAL_CACHE_VCOUNT);
	
	R_ShaderPackAllocLoad(&renderer->shader, str_lit("res/render_2d"));
	R_Attribute attributes[] = { Attribute_Float2, Attribute_Float2, Attribute_Float1, Attribute_Float4, Attribute_Float3, Attribute_Float2 };
	R_PipelineAlloc(&renderer->pipeline, InputAssembly_Triangles, attributes, ArrayCount(attributes), &renderer->shader);
	R_BufferAlloc(&renderer->buffer, BufferFlag_Dynamic | BufferFlag_Type_Vertex);
	R_BufferData(&renderer->buffer, R2D_MAX_INTERNAL_CACHE_VCOUNT * sizeof(R2D_Vertex), nullptr);
	R_PipelineAddBuffer(&renderer->pipeline, &renderer->buffer, ArrayCount(attributes));
	
	R_PipelineBind(&renderer->pipeline);
	i32 textures[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	R_ShaderPackUploadIntArray(&renderer->shader, str_lit("u_tex"), textures, 8);
	mat4 projection = mat4_transpose(mat4_ortho(0, render_size.x, 0, render_size.y, -1, 1000));
	R_ShaderPackUploadMat4(&renderer->shader, str_lit("u_projection"), projection);
	
	R_Texture2DWhite(&renderer->white_texture);
}

void R2D_Free(R2D_Renderer* renderer) {
	R_Texture2DFree(&renderer->white_texture);
	R_BufferFree(&renderer->buffer);
	R_PipelineFree(&renderer->pipeline);
	R_ShaderPackFree(&renderer->shader);
	arena_free(&renderer->arena);
}

void R2D_ResizeProjection(R2D_Renderer* renderer, vec2 render_size) {
	R_PipelineBind(&renderer->pipeline);
	mat4 projection = mat4_transpose(mat4_ortho(0, render_size.x, 0, render_size.y, -1, 1000));
	R_ShaderPackUploadMat4(&renderer->shader, str_lit("u_projection"), projection);
}

void R2D_BeginDraw(R2D_Renderer* renderer) {
	R_BlendAlpha();
	Iterate(renderer->batches, i) {
		R2D_VertexCacheReset(&renderer->batches.elems[i].cache);
		renderer->batches.elems[i].tex_count = 0;
	}
	renderer->current_batch = 0;
}

void R2D_EndDraw(R2D_Renderer* renderer) {
	R_PipelineBind(&renderer->pipeline);
	for (u32 i = 0; i < renderer->current_batch+1; i++) {
		for (u32 t = 0; t < renderer->batches.elems[i].tex_count; t++) {
			R_Texture2DBindTo(renderer->batches.elems[i].textures[t], t);
		}
		R2D_VertexCache* cache = &renderer->batches.elems[i].cache;
		R_BufferUpdate(&renderer->buffer, 0, cache->count * sizeof(R2D_Vertex), (void*) cache->vertices);
		R_Draw(&renderer->pipeline, 0, cache->count);
	}
}

rect R2D_PushCullRect(R2D_Renderer* renderer, rect new_quad) {
	rect ret = renderer->cull_quad;
	renderer->cull_quad = new_quad;
	return ret;
}

void R2D_PopCullRect(R2D_Renderer* renderer, rect old_quad) {
	renderer->cull_quad = old_quad;
}

vec2 R2D_PushOffset(R2D_Renderer* renderer, vec2 new_offset) {
	vec2 ret = renderer->offset;
	renderer->offset = new_offset;
	return ret;
}

void R2D_PopOffset(R2D_Renderer* renderer, vec2 old_offset) {
	renderer->offset = old_offset;
}

void R2D_DrawQuad(R2D_Renderer* renderer, rect quad, R_Texture2D* texture, rect uvs, vec4 color, f32 rounding) {
	quad.x += renderer->offset.x;
	quad.y += renderer->offset.y;
	
	if (!rect_overlaps(quad, renderer->cull_quad)) return;
	
	R2D_Batch* batch = R2D_BatchGetCurrent(renderer, 6, texture);
	i32 idx = R2D_BatchAddTexture(renderer, batch, texture);
	rect uv_culled = rect_uv_cull(quad, uvs, renderer->cull_quad);
	
	R2D_Vertex vertices[] = {
		{
            .pos = vec2_clamp(vec2_init(quad.x, quad.y), renderer->cull_quad),
            .tex_index = idx,
            .tex_coords = vec2_init(uv_culled.x, uv_culled.y),
            .color = color,
			.roundingparams = vec3_init(quad.w, quad.h, rounding),
			.vertid = vec2_init(0, 0),
		},
        {
            .pos = vec2_clamp(vec2_init(quad.x + quad.w, quad.y), renderer->cull_quad),
            .tex_index = idx,
            .tex_coords = vec2_init(uv_culled.x + uv_culled.w, uv_culled.y),
            .color = color,
			.roundingparams = vec3_init(quad.w, quad.h, rounding),
			.vertid = vec2_init(1, 0),
		},
        {
            .pos = vec2_clamp(vec2_init(quad.x + quad.w, quad.y + quad.h), renderer->cull_quad),
            .tex_index = idx,
            .tex_coords = vec2_init(uv_culled.x + uv_culled.w, uv_culled.y + uv_culled.h),
            .color = color,
			.roundingparams = vec3_init(quad.w, quad.h, rounding),
			.vertid = vec2_init(1, 1),
		},
        {
            .pos = vec2_clamp(vec2_init(quad.x, quad.y), renderer->cull_quad),
            .tex_index = idx,
            .tex_coords = vec2_init(uv_culled.x, uv_culled.y),
            .color = color,
			.roundingparams = vec3_init(quad.w, quad.h, rounding),
			.vertid = vec2_init(0, 0),
		},
        {
            .pos = vec2_clamp(vec2_init(quad.x + quad.w, quad.y + quad.h), renderer->cull_quad),
            .tex_index = idx,
            .tex_coords = vec2_init(uv_culled.x + uv_culled.w, uv_culled.y + uv_culled.h),
            .color = color,
			.roundingparams = vec3_init(quad.w, quad.h, rounding),
			.vertid = vec2_init(1, 1),
        },
        {
            .pos = vec2_clamp(vec2_init(quad.x, quad.y + quad.h), renderer->cull_quad),
            .tex_index = idx,
            .tex_coords = vec2_init(uv_culled.x, uv_culled.y + uv_culled.h),
            .color = color,
			.roundingparams = vec3_init(quad.w, quad.h, rounding),
			.vertid = vec2_init(0, 1),
        },
	};
	R2D_VertexCachePush(&batch->cache, vertices, 6);
}

void R2D_DrawQuadC(R2D_Renderer* renderer, rect quad, vec4 color, f32 rounding) {
	R2D_DrawQuad(renderer, quad, &renderer->white_texture, rect_init(0.f, 0.f, 1.f, 1.f), color, rounding);
}

void R2D_DrawQuadT(R2D_Renderer* renderer, rect quad, R_Texture2D* texture, vec4 tint, f32 rounding) {
	R2D_DrawQuad(renderer, quad, texture, rect_init(0.f, 0.f, 1.f, 1.f), tint, rounding);
}

void R2D_DrawQuadST(R2D_Renderer* renderer, rect quad, R_Texture2D* texture, rect uvs, vec4 tint, f32 rounding) {
	R2D_DrawQuad(renderer, quad, texture, uvs, tint, rounding);
}

void R2D_DrawStringC(R2D_Renderer* cb, R2D_FontInfo* fontinfo, vec2 pos, string str, vec4 color) {
    for (u32 i = 0; i < str.size; i++) {
        if (str.str[i] >= 32 && str.str[i] < 128) {
            stbtt_packedchar* info = &fontinfo->cdata[str.str[i] - 32];
            rect uvs = { info->x0 / 512.f, info->y0 / 512.f, (info->x1 - info->x0) / 512.f, (info->y1 - info->y0) / 512.f };
            rect loc = { pos.x + info->xoff, pos.y + info->yoff, info->x1 - info->x0, info->y1 - info->y0 };
            R2D_DrawQuadST(cb, loc, &fontinfo->font_texture, uvs, color, 0);
            pos.x += info->xadvance;
        }
    }
}

void R2D_DrawString(R2D_Renderer* cb, R2D_FontInfo* fontinfo, vec2 pos, string str) {
    for (u32 i = 0; i < str.size; i++) {
        if (str.str[i] >= 32 && str.str[i] < 128) {
            stbtt_packedchar* info = &fontinfo->cdata[str.str[i] - 32];
            rect uvs = {
                info->x0 / 512.f,
                info->y0 / 512.f,
                (info->x1 - info->x0) / 512.f,
                (info->y1 - info->y0) / 512.f
            };
            rect loc = { pos.x + info->xoff, pos.y + info->yoff, info->x1 - info->x0, info->y1 - info->y0 };
            R2D_DrawQuadST(cb, loc, &fontinfo->font_texture, uvs, vec4_init(1.f, 1.f, 1.f, 1.f), 0);
            pos.x += info->xadvance;
        }
    }
}

f32 R2D_GetStringSize(R2D_FontInfo* fontinfo, string str) {
    f32 sz = 0.f;
    for (u32 i = 0; i < str.size; i++) {
        if (str.str[i] >= 32 && str.str[i] < 128) {
            stbtt_packedchar* info = &fontinfo->cdata[str.str[i] - 32];
            sz += info->xadvance;
        }
    }
    return sz;
}

