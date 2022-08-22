#include "helpers.h"
#include "os/os.h"
#include <math.h>
#include <assert.h>
#include <tinyobj/tinyobjloader.h>

static void H_GetFileData(void* ctx, const char* filename, const int is_mtl,
                          const char* obj_filename, char** data, size_t* len) {
    string* s = (string*) ctx;
    *data = (char*) s->str;
    *len = s->size;
}

static void H_CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
    float v10[3];
    float v20[3];
    float len2;
    
    v10[0] = v1[0] - v0[0];
    v10[1] = v1[1] - v0[1];
    v10[2] = v1[2] - v0[2];
    
    v20[0] = v2[0] - v0[0];
    v20[1] = v2[1] - v0[1];
    v20[2] = v2[2] - v0[2];
    
    N[0] = v20[1] * v10[2] - v20[2] * v10[1];
    N[1] = v20[2] * v10[0] - v20[0] * v10[2];
    N[2] = v20[0] * v10[1] - v20[1] * v10[0];
    
    len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
    if (len2 > 0.0f) {
        float len = (float)sqrt((double)len2);
        
        N[0] /= len;
        N[1] /= len;
    }
}

R_Buffer H_LoadObjToBufferVN(string file, u32* count) {
    M_Arena arena;
    arena_init(&arena);
    string content = OS_FileRead(&arena, file);
    
    R_Buffer buffer;
    
    tinyobj_attrib_t attrib;
    tinyobj_shape_t* shapes = nullptr;
    u64 num_shapes;
    tinyobj_material_t* materials = nullptr;
    u64 num_materials;
    
    {
        u32 flags = TINYOBJ_FLAG_TRIANGULATE;
        int ret =
            tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
                              &num_materials, (const char*)file.str,
                              H_GetFileData, &content, flags);
        if (ret != TINYOBJ_SUCCESS) {
            return (R_Buffer) {0};
        }
    }
    {
        u32 face_offset = 0;
        u32 num_triangles = attrib.num_face_num_verts;
        *count = num_triangles * 3;
        u32 stride = 3 + 3;
		R_BufferAlloc(&buffer, BufferFlag_Type_Vertex);
        float* buf_data = malloc(stride * sizeof(float) * num_triangles * 3);
        for (u32 i = 0; i < attrib.num_face_num_verts; i++) {
            u32 f;
            assert(attrib.face_num_verts[i] % 3 == 0); /* assume all triangle faces. */
            
            for (f = 0; f < (size_t)attrib.face_num_verts[i] / 3; f++) {
                float v[3][3];
                float n[3][3];
                
                tinyobj_vertex_index_t idx0 = attrib.faces[face_offset + 3 * f + 0];
                tinyobj_vertex_index_t idx1 = attrib.faces[face_offset + 3 * f + 1];
                tinyobj_vertex_index_t idx2 = attrib.faces[face_offset + 3 * f + 2];
                
                for (u32 k = 0; k < 3; k++) {
                    int f0 = idx0.v_idx;
                    int f1 = idx1.v_idx;
                    int f2 = idx2.v_idx;
                    assert(f0 >= 0);
                    assert(f1 >= 0);
                    assert(f2 >= 0);
                    
                    v[0][k] = attrib.vertices[3 * (size_t)f0 + k];
                    v[1][k] = attrib.vertices[3 * (size_t)f1 + k];
                    v[2][k] = attrib.vertices[3 * (size_t)f2 + k];
                }
                
                if (attrib.num_normals > 0) {
                    int f0 = idx0.vn_idx;
                    int f1 = idx1.vn_idx;
                    int f2 = idx2.vn_idx;
                    if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
                        assert(f0 < (int)attrib.num_normals);
                        assert(f1 < (int)attrib.num_normals);
                        assert(f2 < (int)attrib.num_normals);
                        for (u32 k = 0; k < 3; k++) {
                            n[0][k] = attrib.normals[3 * (size_t)f0 + k];
                            n[1][k] = attrib.normals[3 * (size_t)f1 + k];
                            n[2][k] = attrib.normals[3 * (size_t)f2 + k];
                        }
                    } else { /* normal index is not defined for this face */
                        /* compute geometric normal */
                        H_CalcNormal(n[0], v[0], v[1], v[2]);
                        n[1][0] = n[0][0];
                        n[1][1] = n[0][1];
                        n[1][2] = n[0][2];
                        n[2][0] = n[0][0];
                        n[2][1] = n[0][1];
                        n[2][2] = n[0][2];
                    }
                } else {
                    /* compute geometric normal */
                    H_CalcNormal(n[0], v[0], v[1], v[2]);
                    n[1][0] = n[0][0];
                    n[1][1] = n[0][1];
                    n[1][2] = n[0][2];
                    n[2][0] = n[0][0];
                    n[2][1] = n[0][1];
                    n[2][2] = n[0][2];
                }
                
                for (u32 k = 0; k < 3; k++) {
                    buf_data[(3 * i + k) * stride + 0] = v[k][0];
                    buf_data[(3 * i + k) * stride + 1] = v[k][1];
                    buf_data[(3 * i + k) * stride + 2] = v[k][2];
                    buf_data[(3 * i + k) * stride + 3] = n[k][0];
                    buf_data[(3 * i + k) * stride + 4] = n[k][1];
                    buf_data[(3 * i + k) * stride + 5] = n[k][2];
                }
            }
            
            face_offset += (size_t)attrib.face_num_verts[i];
        }
		
		R_BufferData(&buffer, stride * sizeof(float) * num_triangles * 3, buf_data);
    }
    
    
    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
    
    arena_free(&arena);
    return buffer;
}

R_Buffer H_LoadObjToBufferVNCs(string file, u32* count, vec4 color) {
    M_Arena arena;
    arena_init(&arena);
    string content = OS_FileRead(&arena, file);
    
    R_Buffer buffer;
    
    tinyobj_attrib_t attrib;
    tinyobj_shape_t* shapes = nullptr;
    u64 num_shapes;
    tinyobj_material_t* materials = nullptr;
    u64 num_materials;
    
    {
        u32 flags = TINYOBJ_FLAG_TRIANGULATE;
        int ret =
            tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
                              &num_materials, (const char*)file.str,
                              H_GetFileData, &content, flags);
        if (ret != TINYOBJ_SUCCESS) {
            return (R_Buffer) {0};
        }
    }
    {
        u32 face_offset = 0;
        u32 num_triangles = attrib.num_face_num_verts;
        *count = num_triangles * 3;
        u32 stride = 3 + 3 + 4;
		R_BufferAlloc(&buffer, BufferFlag_Type_Vertex);
		
		float* buf_data = malloc(stride * sizeof(float) * num_triangles * 3);
        for (u32 i = 0; i < attrib.num_face_num_verts; i++) {
            u32 f;
            assert(attrib.face_num_verts[i] % 3 == 0); /* assume all triangle faces. */
            
            for (f = 0; f < (size_t)attrib.face_num_verts[i] / 3; f++) {
                float v[3][3];
                float n[3][3];
                
                tinyobj_vertex_index_t idx0 = attrib.faces[face_offset + 3 * f + 0];
                tinyobj_vertex_index_t idx1 = attrib.faces[face_offset + 3 * f + 1];
                tinyobj_vertex_index_t idx2 = attrib.faces[face_offset + 3 * f + 2];
                
                for (u32 k = 0; k < 3; k++) {
                    int f0 = idx0.v_idx;
                    int f1 = idx1.v_idx;
                    int f2 = idx2.v_idx;
                    assert(f0 >= 0);
                    assert(f1 >= 0);
                    assert(f2 >= 0);
                    
                    v[0][k] = attrib.vertices[3 * (size_t)f0 + k];
                    v[1][k] = attrib.vertices[3 * (size_t)f1 + k];
                    v[2][k] = attrib.vertices[3 * (size_t)f2 + k];
                }
                
                if (attrib.num_normals > 0) {
                    int f0 = idx0.vn_idx;
                    int f1 = idx1.vn_idx;
                    int f2 = idx2.vn_idx;
                    if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
                        assert(f0 < (int)attrib.num_normals);
                        assert(f1 < (int)attrib.num_normals);
                        assert(f2 < (int)attrib.num_normals);
                        for (u32 k = 0; k < 3; k++) {
                            n[0][k] = attrib.normals[3 * (size_t)f0 + k];
                            n[1][k] = attrib.normals[3 * (size_t)f1 + k];
                            n[2][k] = attrib.normals[3 * (size_t)f2 + k];
                        }
                    } else { /* normal index is not defined for this face */
                        /* compute geometric normal */
                        H_CalcNormal(n[0], v[0], v[1], v[2]);
                        n[1][0] = n[0][0];
                        n[1][1] = n[0][1];
                        n[1][2] = n[0][2];
                        n[2][0] = n[0][0];
                        n[2][1] = n[0][1];
                        n[2][2] = n[0][2];
                    }
                } else {
                    /* compute geometric normal */
                    H_CalcNormal(n[0], v[0], v[1], v[2]);
                    n[1][0] = n[0][0];
                    n[1][1] = n[0][1];
                    n[1][2] = n[0][2];
                    n[2][0] = n[0][0];
                    n[2][1] = n[0][1];
                    n[2][2] = n[0][2];
                }
                
                for (u32 k = 0; k < 3; k++) {
                    buf_data[(3 * i + k) * stride + 0] = v[k][0];
                    buf_data[(3 * i + k) * stride + 1] = v[k][1];
                    buf_data[(3 * i + k) * stride + 2] = v[k][2];
                    buf_data[(3 * i + k) * stride + 3] = n[k][0];
                    buf_data[(3 * i + k) * stride + 4] = n[k][1];
                    buf_data[(3 * i + k) * stride + 5] = n[k][2];
                    buf_data[(3 * i + k) * stride + 6] = color.x;
                    buf_data[(3 * i + k) * stride + 7] = color.y;
                    buf_data[(3 * i + k) * stride + 8] = color.z;
                    buf_data[(3 * i + k) * stride + 9] = color.w;
                }
            }
            
            face_offset += (size_t)attrib.face_num_verts[i];
        }
		R_BufferData(&buffer, stride * sizeof(float) * num_triangles * 3, buf_data);
    }
    
    
    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
    
    arena_free(&arena);
    return buffer;
}
