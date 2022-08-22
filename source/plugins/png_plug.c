#include "base/log.h"
#include "base/vmath.h"
#include "opt/render_2d.h"
#include <math.h>

static R2D_FontInfo finfo = {0};
static R_Texture2D texture = {0};
static string fp = {0};

dll_export string_array Extensions(M_Arena* arena) {
	string exts[] = {
		str_lit("png")
	};
	return string_static_array_make(arena, exts, 1);
}

dll_export void Init(string filepath) {
	fp = filepath;
	R2D_FontLoad(&finfo, str_lit("res/Inconsolata.ttf"), 32.f);
	R_Texture2DAllocLoad(&texture, filepath, TextureResize_Linear, TextureResize_Linear, TextureWrap_Repeat, TextureWrap_Repeat);
}

dll_export void Render(R2D_Renderer* renderer) {
	R2D_DrawStringC(renderer, &finfo, (vec2) { 300 - (R2D_GetStringSize(&finfo, fp)/2.f), 60 }, fp, Color_Green);
	R2D_DrawQuadT(renderer, (rect) { 100, 100, 400, 400 }, &texture, Color_White, 0);
}

dll_export void Free() {
	R2D_FontFree(&finfo);
}
