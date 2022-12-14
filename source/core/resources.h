/* date = July 3rd 2022 0:06 pm */

#ifndef RESOURCES_H
#define RESOURCES_H

#include "defines.h"
#include "base/str.h"
#include "base/ds.h"
#include "base/vmath.h"
#include "os/window.h"

//~ Buffers

typedef u32 R_BufferFlags;
enum {
	BufferFlag_Dynamic = 0x1,
	
	BufferFlag_Type_Vertex = 0x2,
	BufferFlag_Type_Index = 0x4,
	BufferFlag_Type_Uniform = 0x8,
};

typedef struct R_Buffer {
	R_BufferFlags flags;
	u64 v[2];
} R_Buffer;

dll_plugin_api void R_BufferAlloc(R_Buffer* buf, R_BufferFlags flags);
dll_plugin_api void R_BufferData(R_Buffer* buf, u64 size, void* data);
dll_plugin_api void R_BufferUpdate(R_Buffer* _buf, u64 offset, u64 size, void* data);
dll_plugin_api void R_BufferFree(R_Buffer* buf);

//~ Shaders

typedef u32 R_ShaderType;
enum {
	ShaderType_Vertex,
	ShaderType_Fragment,
	ShaderType_Geometry,
	
	ShaderType_MAX,
};

typedef struct R_Shader {
	R_ShaderType type;
	u64 v[1];
} R_Shader;

typedef struct R_ShaderPack {
	u64 v[4];
} R_ShaderPack;

dll_plugin_api void R_ShaderAlloc(R_Shader* shader, string data, R_ShaderType type);
dll_plugin_api void R_ShaderAllocLoad(R_Shader* shader, string fp, R_ShaderType type);
dll_plugin_api void R_ShaderFree(R_Shader* shader);

dll_plugin_api void R_ShaderPackAlloc(R_ShaderPack* pack, R_Shader* shaders, u32 shader_count);
dll_plugin_api void R_ShaderPackAllocLoad(R_ShaderPack* pack, string fp_prefix);
dll_plugin_api void R_ShaderPackFree(R_ShaderPack* pack);

dll_plugin_api void R_ShaderPackUploadMat4(R_ShaderPack* pack, string name, mat4 mat);
dll_plugin_api void R_ShaderPackUploadInt(R_ShaderPack* pack, string name, i32 val);
dll_plugin_api void R_ShaderPackUploadIntArray(R_ShaderPack* _pack, string name, i32* vals, u32 count);
dll_plugin_api void R_ShaderPackUploadFloat(R_ShaderPack* pack, string name, f32 val);
dll_plugin_api void R_ShaderPackUploadVec4(R_ShaderPack* pack, string name, vec4 val);

//~ Pipelines (VAOs)

typedef u32 R_InputAssembly;
enum {
	InputAssembly_Triangles,
	InputAssembly_Lines,
	
	InputAssembly_MAX,
};

typedef u32 R_Attribute;
enum {
	Attribute_Float1,
	Attribute_Float2,
	Attribute_Float3,
	Attribute_Float4,
	Attribute_Integer1,
	Attribute_Integer2,
	Attribute_Integer3,
	Attribute_Integer4,
	
	Attribute_MAX,
};

typedef struct R_Pipeline {
	R_InputAssembly assembly;
	R_Attribute* attributes;
	R_ShaderPack* shader;
	u32 attribute_count;
	
	u64 v[2];
} R_Pipeline;

dll_plugin_api void R_PipelineAlloc(R_Pipeline* _in, R_InputAssembly assembly, R_Attribute* attributes, u32 attribute_count, R_ShaderPack* shader);
dll_plugin_api void R_PipelineAddBuffer(R_Pipeline* in, R_Buffer* _buf, u32 attribute_count);
dll_plugin_api void R_PipelineBind(R_Pipeline* in);
dll_plugin_api void R_PipelineFree(R_Pipeline* in);

//~ Textures

typedef u32 R_TextureWrapParam;
enum {
	TextureWrap_ClampToEdge,
	TextureWrap_ClampToBorder,
	TextureWrap_Repeat,
	TextureWrap_MirroredRepeat,
	TextureWrap_MirrorClampToEdge,
	
	TextureWrap_MAX
};

typedef u32 R_TextureResizeParam;
enum {
	TextureResize_Nearest,
	TextureResize_Linear,
	TextureResize_LinearMipmapLinear,
	TextureResize_LinearMipmapNearest,
	TextureResize_NearestMipmapLinear,
	TextureResize_NearestMipmapNearest,
	
	TextureResize_MAX,
};

typedef u32 R_TextureFormat;
enum {
	TextureFormat_Invalid,
	
	TextureFormat_RInteger,
	TextureFormat_R,
	TextureFormat_RG,
	TextureFormat_RGB,
	TextureFormat_RGBA,
	TextureFormat_DepthStencil,
	
	TextureFormat_MAX,
};

typedef i32 R_TextureChannel;
enum {
	TextureChannel_Zero,
	TextureChannel_One,
	TextureChannel_R,
	TextureChannel_G,
	TextureChannel_B,
	TextureChannel_A,
	TextureChannel_MAX,
};

typedef struct R_Texture2D {
	u32 width;
	u32 height;
	
	R_TextureFormat format;
	R_TextureResizeParam min;
	R_TextureResizeParam mag;
	R_TextureWrapParam wrap_s;
	R_TextureWrapParam wrap_t;
	
	u64 v[1];
} R_Texture2D;

dll_plugin_api void R_Texture2DAlloc(R_Texture2D* texture, R_TextureFormat format, u32 width, u32 height, R_TextureResizeParam min, R_TextureResizeParam mag, R_TextureWrapParam wrap_s, R_TextureWrapParam wrap_t);
dll_plugin_api void R_Texture2DAllocLoad(R_Texture2D* texture, string filepath, R_TextureResizeParam min, R_TextureResizeParam mag, R_TextureWrapParam wrap_s, R_TextureWrapParam wrap_t);
dll_plugin_api void R_Texture2DSwizzle(R_Texture2D* texture, i32* swizzles);
dll_plugin_api void R_Texture2DData(R_Texture2D* texture, void* data);
dll_plugin_api void R_Texture2DWhite(R_Texture2D* texture);
dll_plugin_api b8   R_Texture2DEquals(R_Texture2D* a, R_Texture2D* b);

dll_plugin_api void R_Texture2DBindTo(R_Texture2D* texture, u32 slot);
dll_plugin_api void R_Texture2DFree(R_Texture2D* texture);

//~ Framebuffer

typedef struct R_Framebuffer {
	u32 width;
	u32 height;
	
	R_Texture2D* color_attachments;
	u32 color_attachment_count;
	R_Texture2D depth_attachment;
	
	u64 v[1];
} R_Framebuffer;

dll_plugin_api void R_FramebufferCreate(R_Framebuffer* _framebuffer, u32 width, u32 height, R_Texture2D* color_attachments, u32 color_attachment_count, R_Texture2D depth_attachment);
dll_plugin_api void R_FramebufferBind(R_Framebuffer* framebuffer);
dll_plugin_api void R_FramebufferBindScreen(void);
dll_plugin_api void R_FramebufferBlitToScreen(OS_Window* window, R_Framebuffer* framebuffer);
dll_plugin_api void R_FramebufferReadPixel(R_Framebuffer* _framebuffer, u32 attachment, u32 x, u32 y,
										   void* data);
dll_plugin_api void R_FramebufferResize(R_Framebuffer* framebuffer, u32 new_width, u32 new_height);
dll_plugin_api void R_FramebufferFree(R_Framebuffer* framebuffer);

//~ Other

typedef u32 R_BufferMask;
enum {
	BufferMask_Color = 0x01,
	BufferMask_Depth = 0x02,
	BufferMask_Stencil = 0x04,
};

typedef u32 R_CullFace;
enum {
	CullFace_None,
	CullFace_Back,
	CullFace_Front,
	CullFace_MAX,
};

dll_plugin_api void R_Clear(R_BufferMask buffer_mask);
dll_plugin_api void R_ClearColor(f32 r, f32 g, f32 b, f32 a);

// TODO(voxel): Move all this to pipelines
dll_plugin_api void R_Viewport(i32 x, i32 y, i32 w, i32 h);
dll_plugin_api void R_BlendDisable(void);
dll_plugin_api void R_BlendAlpha(void);
dll_plugin_api void R_DepthEnable(void);
dll_plugin_api void R_DepthDisable(void);
dll_plugin_api void R_Cull(R_CullFace to_cull);

dll_plugin_api void R_Draw(R_Pipeline* pipeline, u32 start, u32 count);

#endif //RESOURCES_H
