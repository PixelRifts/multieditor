#include "defines.h"
#include "base/str.h"
#include "base/mem.h"
#include "os/os.h"

#include "opt/render_2d.h"
#include "opt/ui.h"
#include "os/input.h"
#include "core/helpers.h"
#include <stdio.h>
#include <math.h>

typedef struct Camera3D {
    vec3 pos;
    f32 pitch;
    f32 yaw;
    f32 roll;
    f32 zoom;
    f32 target_zoom;
    f32 angle_around;
} Camera3D;

Camera3D Camera3DInit(vec3 init_pos, f32 pitch, f32 yaw, f32 roll) {
    return (Camera3D) {
        init_pos,
        pitch, yaw, roll,
        -8, -8, 0,
    };
}

mat4 Camera3DView(Camera3D* cam) {
    vec3 neg_pos = { -cam->pos.x, -cam->pos.y, -cam->pos.z };
    quat rot = quat_identity();
    rot = quat_rotate_axis(rot, 1, 0, 0, -cam->pitch * DEG_TO_RAD);
    rot = quat_mul(rot, quat_rotate_axis(rot, 0, 1, 0, -cam->yaw * DEG_TO_RAD));
    rot = quat_mul(rot, quat_rotate_axis(rot, 0, 0, 1, -cam->roll * DEG_TO_RAD));
    mat4 ret = mat4_translate(neg_pos);
    ret = mat4_mul(ret, quat_to_rotation_mat(rot));
    return ret;
}

void Camera3DControls(Camera3D* cam, f32 dt) {
    f32 zoom_level = OS_InputGetMouseScrollY() * 200.f * dt;
    cam->target_zoom += zoom_level;
    cam->target_zoom = Clamp(-20.f, cam->target_zoom, 0.f);
    animate_f32exp(&cam->zoom, cam->target_zoom, 10, dt);
    
    if (OS_InputButton(Input_MouseButton_Left)) {
        f32 pitch_change = OS_InputGetMouseDY() * dt * 0.4f;
        cam->pitch -= pitch_change;
        f32 angle_change = OS_InputGetMouseDX() * dt * 0.3f;
        cam->angle_around -= angle_change;
    }
    
    cam->pitch = Clamp(-90, cam->pitch, 90);
    
    f32 horiz = cam->zoom * cosf(cam->pitch * DEG_TO_RAD);
    f32 verti = cam->zoom * sinf(cam->pitch * DEG_TO_RAD);
    
    f32 offz = horiz * cosf(cam->angle_around * DEG_TO_RAD);
    f32 offx = horiz * sinf(cam->angle_around * DEG_TO_RAD);
    
    cam->pos.x = offx;
    cam->pos.y = verti;
    cam->pos.z = offz;
    
    cam->yaw = -180 + cam->angle_around;
}

typedef u32 PackingType;
enum {
    PackType_SimpleCubic,
    PackType_FaceCentered,
    PackType_BodyCentered,
};

typedef struct solidstate_file {
	PackingType type;
	mat4 projection;
    Camera3D cam;
} solidstate_file;

static string fp;
static M_Arena arena = {0};
static solidstate_file data = {0};

dll_export void Init(string filepath);
dll_export void Free(void);

void SwitchScene(PackingType type) {
	data.type = type;
	Free();
	Init(fp);
}

#define OBJECT_COUNT 14
#define LINE_COUNT (12 + 24 + 24 + 40 + 20 + 24)

typedef struct LineVertex {
    vec3 pos;
    vec4 color;
} LineVertex;

typedef struct solid_state_State {
    PackingType curr_type;
    
    R_ShaderPack three_dim;
    R_ShaderPack three_dim_lines;
    R_Buffer sphere_buffer;
    R_Pipeline sphere_vertex_array;
    u32 sphere_vertex_count;
    
    R_Pipeline* objects[OBJECT_COUNT];
    u32 obj_v_count[OBJECT_COUNT];
    mat4 obj_transforms[OBJECT_COUNT];
    vec4 obj_colors[OBJECT_COUNT];
    
    LineVertex lines_data[LINE_COUNT * 2];
    u32 line_v_count;
    R_Pipeline lines_vertex_array;
    R_Buffer lines_buffer;
    
    R_ShaderPack two_dim;
    R_Buffer fullscreen_buffer;
    R_Pipeline fullscreen_vertex_array;
    
    R_Framebuffer fbo;
    R_Texture2D fbo_color0;
    R_Texture2D fbo_color1;
    R_Texture2D fbo_depth;
    
    u32 framebuffer_width;
    u32 framebuffer_height;
    u32 selected_obj_id;
    u32 pressed_loc_x;
    u32 pressed_loc_y;
    b8 threexthree;
    f32 sphere_radius;
    vec2 sphere_radius_range;
} solid_state_State;

static solid_state_State v_solid_state_state = {0};

void PushLine(vec3 a, vec3 b, vec4 c) {
    v_solid_state_state.lines_data[v_solid_state_state.line_v_count].pos = a;
    v_solid_state_state.lines_data[v_solid_state_state.line_v_count++].color = c;
    v_solid_state_state.lines_data[v_solid_state_state.line_v_count].pos = b;
    v_solid_state_state.lines_data[v_solid_state_state.line_v_count++].color = c;
}

void Push3X3Lines() {
    v_solid_state_state.line_v_count = 0;
    // Cube Center
    PushLine(vec3_init(1, 1, 1), vec3_init(1, 1, -1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(1, 1, -1), vec3_init(1, -1, -1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(1, -1, -1), vec3_init(1, -1, 1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(1, -1, 1), vec3_init(1, 1, 1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(-1, 1, 1), vec3_init(-1, 1, -1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(-1, 1, -1), vec3_init(-1, -1, -1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(-1, -1, -1), vec3_init(-1, -1, 1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(-1, -1, 1), vec3_init(-1, 1, 1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(1, 1, 1), vec3_init(-1, 1, 1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(1, 1, -1), vec3_init(-1, 1, -1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(1, -1, 1), vec3_init(-1, -1, 1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    PushLine(vec3_init(1, -1, -1), vec3_init(-1, -1, -1), vec4_init(0.8f, 0.2f, 0.3f, 1.0f));
    
    if (!v_solid_state_state.threexthree) return;
    
    // X Faces
    for (i32 i = -1; i != 3; i += 2) {
        PushLine(vec3_init(3 * i, -1, -1), vec3_init(1 * i, -1, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -1, 1),  vec3_init(1 * i, -1, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 1, -1),  vec3_init(1 * i, 1, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 1, 1),   vec3_init(1 * i, 1, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -1, -1), vec3_init(3 * i, -1, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -1, 1),  vec3_init(3 * i, 1, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 1, 1),   vec3_init(3 * i, 1, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 1, -1),  vec3_init(3 * i, -1, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
    }
    
    // Y Faces
    for (i32 i = -1; i != 3; i += 2) {
        PushLine(vec3_init(-1, 3 * i, -1), vec3_init(-1, 1 * i, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, 3 * i, 1),  vec3_init(-1, 1 * i, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, 3 * i, -1),  vec3_init(1, 1 * i, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, 3 * i, 1),   vec3_init(1, 1 * i, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, 3 * i, -1), vec3_init(-1, 3 * i, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, 3 * i, 1),  vec3_init(1, 3 * i, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, 3 * i, 1),   vec3_init(1, 3 * i, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, 3 * i, -1),  vec3_init(-1, 3 * i, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
    }
    
    // Z Faces
    for (i32 i = -1; i != 3; i += 2) {
        PushLine(vec3_init(-1, -1, 3 * i), vec3_init(-1, -1, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, 1, 3 * i),  vec3_init(-1, 1, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, -1, 3 * i),  vec3_init(1, -1, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, 1, 3 * i),   vec3_init(1, 1, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, -1, 3 * i), vec3_init(-1, 1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, 1, 3 * i),  vec3_init(1, 1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, 1, 3 * i),   vec3_init(1, -1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, -1, 3 * i),  vec3_init(-1, -1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
    }
    
    // Edge Cubes
    for (i32 i = -1; i != 3; i += 2) {
        PushLine(vec3_init(3 * i, 3, -1), vec3_init(3 * i, 1, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 3, -1), vec3_init(1 * i, 3, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 3, 1), vec3_init(1 * i, 3, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 3, 1), vec3_init(3 * i, 1, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 3, -1), vec3_init(3 * i, 3, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        
        PushLine(vec3_init(3 * i, -3, -1), vec3_init(3 * i, -1, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -3, -1), vec3_init(1 * i, -3, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -3, 1), vec3_init(1 * i, -3, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -3, 1), vec3_init(3 * i, -1, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -3, -1), vec3_init(3 * i, -3, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        
        PushLine(vec3_init(-1, 3, 3 * i), vec3_init(-1, 1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, 3, 3 * i), vec3_init(-1, 3, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, 3, 3 * i), vec3_init(1, 3, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, 3, 3 * i), vec3_init(1, 1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, 3, 3 * i), vec3_init(1, 3, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        
        PushLine(vec3_init(-1, -3, 3 * i), vec3_init(-1, -1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, -3, 3 * i), vec3_init(-1, -3, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, -3, 3 * i), vec3_init(1, -3, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(1, -3, 3 * i), vec3_init(1, -1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-1, -3, 3 * i), vec3_init(1, -3, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        
        PushLine(vec3_init(3, -1, 3 * i), vec3_init(1, -1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3, -1, 3 * i), vec3_init(3, -1, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3, 1, 3 * i), vec3_init(3, 1, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3, 1, 3 * i), vec3_init(1, 1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3, -1, 3 * i), vec3_init(3, 1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        
        PushLine(vec3_init(-3, -1, 3 * i), vec3_init(-1, -1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-3, -1, 3 * i), vec3_init(-3, -1, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-3, 1, 3 * i), vec3_init(-3, 1, 1 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-3, 1, 3 * i), vec3_init(-1, 1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(-3, -1, 3 * i), vec3_init(-3, 1, 3 * i), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
    }
    
    // Corner Cubes
    for (i32 i = -1; i != 3; i += 2) {
        PushLine(vec3_init(3 * i, 3, 3), vec3_init(1 * i, 3, 3), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 3, 3), vec3_init(3 * i, 1, 3), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 3, 3), vec3_init(3 * i, 3, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        
        PushLine(vec3_init(3 * i, 3, -3), vec3_init(1 * i, 3, -3), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 3, -3), vec3_init(3 * i, 1, -3), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, 3, -3), vec3_init(3 * i, 3, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        
        PushLine(vec3_init(3 * i, -3, 3), vec3_init(1 * i, -3, 3), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -3, 3), vec3_init(3 * i, -1, 3), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -3, 3), vec3_init(3 * i, -3, 1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        
        PushLine(vec3_init(3 * i, -3, -3), vec3_init(1 * i, -3, -3), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -3, -3), vec3_init(3 * i, -1, -3), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
        PushLine(vec3_init(3 * i, -3, -3), vec3_init(3 * i, -3, -1), vec4_init(0.2f, 0.8f, 0.3f, 0.7f));
    }
}

void RefreshTransforms() {
    f32 r = v_solid_state_state.sphere_radius;
    switch (v_solid_state_state.curr_type) {
        case PackType_SimpleCubic:
        v_solid_state_state.obj_transforms[0] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, -1.f, -1.f)));
        v_solid_state_state.obj_transforms[1] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, -1.f, 1.f)));
        v_solid_state_state.obj_transforms[2] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, 1.f, -1.f)));
        v_solid_state_state.obj_transforms[3] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, 1.f, 1.f)));
        v_solid_state_state.obj_transforms[4] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, -1.f, -1.f)));
        v_solid_state_state.obj_transforms[5] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, -1.f, 1.f)));
        v_solid_state_state.obj_transforms[6] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, 1.f, -1.f)));
        v_solid_state_state.obj_transforms[7] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, 1.f, 1.f)));
        break;
        
        case PackType_BodyCentered:
        v_solid_state_state.obj_transforms[0] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, -1.f, -1.f)));
        v_solid_state_state.obj_transforms[1] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, -1.f, 1.f)));
        v_solid_state_state.obj_transforms[2] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, 1.f, -1.f)));
        v_solid_state_state.obj_transforms[3] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, 1.f, 1.f)));
        v_solid_state_state.obj_transforms[4] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, -1.f, -1.f)));
        v_solid_state_state.obj_transforms[5] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, -1.f, 1.f)));
        v_solid_state_state.obj_transforms[6] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, 1.f, -1.f)));
        v_solid_state_state.obj_transforms[7] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, 1.f, 1.f)));
        v_solid_state_state.obj_transforms[8] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(0.f, 0.f, 0.f)));
        break;
        
        case PackType_FaceCentered:
        v_solid_state_state.obj_transforms[0] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, -1.f, -1.f)));
        v_solid_state_state.obj_transforms[1] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, -1.f, 1.f)));
        v_solid_state_state.obj_transforms[2] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, 1.f, -1.f)));
        v_solid_state_state.obj_transforms[3] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, 1.f, 1.f)));
        v_solid_state_state.obj_transforms[4] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, -1.f, -1.f)));
        v_solid_state_state.obj_transforms[5] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, -1.f, 1.f)));
        v_solid_state_state.obj_transforms[6] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, 1.f, -1.f)));
        v_solid_state_state.obj_transforms[7] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, 1.f, 1.f)));
        v_solid_state_state.obj_transforms[8] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(0.f, 0.f, 1.f)));
        v_solid_state_state.obj_transforms[9] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(0.f, 0.f, -1.f)));
        v_solid_state_state.obj_transforms[10] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(0.f, 1.f, 0.f)));
        v_solid_state_state.obj_transforms[11] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(0.f, -1.f, 0.f)));
        v_solid_state_state.obj_transforms[12] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(1.f, 0.f, 0.f)));
        v_solid_state_state.obj_transforms[13] = mat4_mul(mat4_scale(vec3_init(r, r, r)), mat4_translate(vec3_init(-1.f, 0.f, 0.f)));
        break;
    }
}


dll_export string_array Extensions(M_Arena* arena) {
	string exts[] = {
		str_lit("solidstate")
	};
	return string_static_array_make(arena, exts, 1);
}

dll_export void Init(string filepath) {
	arena_init(&arena);
    v_solid_state_state = (solid_state_State) {0};
	
	fp = filepath;
	string strdata = OS_FileRead(&arena, filepath);
	if (strdata.size == sizeof(solidstate_file)) {
		memmove(&data, strdata.str, sizeof(solidstate_file));
	} else {
		data.type = PackType_SimpleCubic;
		data.cam = Camera3DInit(vec3_init(0, 0, 4), 0, 0, 0);
		data.projection = mat4_perspective(80, 1080.f / 720.f, 1, 1000);
	}
	
	PackingType type = data.type;
	
    v_solid_state_state.curr_type = type;
    v_solid_state_state.threexthree = false;
    R_DepthEnable();
    R_BlendAlpha();
    
    v_solid_state_state.selected_obj_id = 0;
    v_solid_state_state.framebuffer_width = 1080;
    v_solid_state_state.framebuffer_height = 720;
	R_Texture2DAlloc(&v_solid_state_state.fbo_color0, TextureFormat_RGBA, 1080, 720, TextureResize_Nearest, TextureResize_Nearest, TextureWrap_Repeat, TextureWrap_Repeat);
	R_Texture2DAlloc(&v_solid_state_state.fbo_color1, TextureFormat_RInteger, 1080, 720, TextureResize_Nearest, TextureResize_Nearest, TextureWrap_Repeat, TextureWrap_Repeat);
	R_Texture2DAlloc(&v_solid_state_state.fbo_depth, TextureFormat_DepthStencil, 1080, 720, TextureResize_Nearest, TextureResize_Nearest, TextureWrap_Repeat, TextureWrap_Repeat);
    R_Texture2D color_attachments[] = {
        v_solid_state_state.fbo_color0,
        v_solid_state_state.fbo_color1,
    };
	R_FramebufferCreate(&v_solid_state_state.fbo, 1080, 720, color_attachments, 2, v_solid_state_state.fbo_depth);
    
	R_ShaderPackAllocLoad(&v_solid_state_state.three_dim, str_lit("res/three_dim"));
    R_ShaderPackAllocLoad(&v_solid_state_state.three_dim_lines, str_lit("res/three_dim_lines"));
    R_ShaderPackAllocLoad(&v_solid_state_state.two_dim, str_lit("res/two_dim"));
    
    v_solid_state_state.sphere_buffer = H_LoadObjToBufferVN(str_lit("res/ball.obj"), &v_solid_state_state.sphere_vertex_count);
    R_Attribute threedim_attribs[] = { Attribute_Float3, Attribute_Float3 };
	R_PipelineAlloc(&v_solid_state_state.sphere_vertex_array, InputAssembly_Triangles, threedim_attribs, 2, &v_solid_state_state.three_dim);
    R_PipelineAddBuffer(&v_solid_state_state.sphere_vertex_array, &v_solid_state_state.sphere_buffer, 2);
	R_PipelineBind(&v_solid_state_state.sphere_vertex_array);
	R_ShaderPackUploadMat4(&v_solid_state_state.three_dim, str_lit("u_projection"), data.projection);
	
	
	for (u32 i = 0; i < OBJECT_COUNT; i++) {
		v_solid_state_state.objects[i] = &v_solid_state_state.sphere_vertex_array;
		v_solid_state_state.obj_v_count[i] = v_solid_state_state.sphere_vertex_count;
		v_solid_state_state.obj_colors[i] = (vec4) { 0.2f, 0.3f, 0.8f, 0.8f };
	}
	
	switch (type) {
		case PackType_SimpleCubic:
		v_solid_state_state.sphere_radius = 1.0f;
		v_solid_state_state.sphere_radius_range.x = 0.2f;
		v_solid_state_state.sphere_radius_range.y = 1.0f;
		break;
		
		case PackType_FaceCentered:
		v_solid_state_state.sphere_radius = sqrt(2) / 2.f;
		v_solid_state_state.sphere_radius_range.x = 0.2f;
		v_solid_state_state.sphere_radius_range.y = sqrt(2) / 2.f;
		break;
		
		case PackType_BodyCentered:
		v_solid_state_state.sphere_radius = sqrt(3) / 2.f;
		v_solid_state_state.sphere_radius_range.x = 0.2f;
		v_solid_state_state.sphere_radius_range.y = sqrt(3) / 2.f;
		break;
	}
	RefreshTransforms();
	
	f32 fullscreen_quad[] = {
		-1.f, -1.f,  0.f, 0.f,  1.f, 1.f, 1.f, 1.f,
		1.f, -1.f,   1.f, 0.f,  1.f, 1.f, 1.f, 1.f,
		1.f,  1.f,   1.f, 1.f,  1.f, 1.f, 1.f, 1.f,
		
		-1.f, -1.f,  0.f, 0.f,  1.f, 1.f, 1.f, 1.f,
		1.f,  1.f,   1.f, 1.f,  1.f, 1.f, 1.f, 1.f,
		-1.f,  1.f,  0.f, 1.f,  1.f, 1.f, 1.f, 1.f,
	};
	R_BufferAlloc(&v_solid_state_state.fullscreen_buffer, BufferFlag_Type_Vertex);
	R_BufferData(&v_solid_state_state.fullscreen_buffer, sizeof(fullscreen_quad), fullscreen_quad);
	
	R_Attribute twodim_attribs[] = { Attribute_Float2, Attribute_Float2, Attribute_Float4 };
	R_PipelineAlloc(&v_solid_state_state.fullscreen_vertex_array, InputAssembly_Triangles,
					twodim_attribs, 3, &v_solid_state_state.two_dim);
	R_PipelineAddBuffer(&v_solid_state_state.fullscreen_vertex_array, &v_solid_state_state.fullscreen_buffer, 3);
	R_PipelineBind(&v_solid_state_state.fullscreen_vertex_array);
	R_ShaderPackUploadMat4(&v_solid_state_state.two_dim, str_lit("u_projection"), mat4_identity());
	R_ShaderPackUploadMat4(&v_solid_state_state.two_dim, str_lit("u_view"), mat4_identity());
	
	R_BufferAlloc(&v_solid_state_state.lines_buffer, BufferFlag_Type_Vertex | BufferFlag_Dynamic);
	Push3X3Lines();
	R_BufferData(&v_solid_state_state.lines_buffer, sizeof(LineVertex) * LINE_COUNT * 2, v_solid_state_state.lines_data);
	
	R_Attribute threedimlines_attribs[] = { Attribute_Float3, Attribute_Float4 };
	R_PipelineAlloc(&v_solid_state_state.lines_vertex_array, InputAssembly_Lines, threedimlines_attribs, 2, &v_solid_state_state.three_dim_lines);
	R_PipelineAddBuffer(&v_solid_state_state.lines_vertex_array, &v_solid_state_state.lines_buffer, 2);
	R_PipelineBind(&v_solid_state_state.lines_vertex_array);
	R_ShaderPackUploadMat4(&v_solid_state_state.three_dim_lines, str_lit("u_projection"), data.projection);
	R_ShaderPackUploadMat4(&v_solid_state_state.three_dim_lines, str_lit("u_transform"), mat4_identity());
	
}

dll_export void CustomRender(void) {
	R_FramebufferBind(&v_solid_state_state.fbo);
    R_BlendAlpha();
    R_Clear(BufferMask_Color | BufferMask_Depth);
    R_DepthEnable();
    
    // Move to render.c api
    R_Cull(CullFace_Back);
    
    R_PipelineBind(v_solid_state_state.objects[0]);
    mat4 view = Camera3DView(&data.cam);
    R_ShaderPackUploadMat4(&v_solid_state_state.three_dim, str_lit("u_view"), view);
    for (u32 i = 0; i < OBJECT_COUNT; i++) {
        R_PipelineBind(v_solid_state_state.objects[i]);
        R_ShaderPackUploadMat4(&v_solid_state_state.three_dim, str_lit("u_transform"),
							   v_solid_state_state.obj_transforms[i]);
        vec4 c = v_solid_state_state.obj_colors[i];
        c = v_solid_state_state.selected_obj_id == i + 1 ? vec4_add(c, vec4_init(0.15f, 0.15f, 0.15f, 0.0f)) : c;
        R_ShaderPackUploadVec4(&v_solid_state_state.three_dim, str_lit("u_color"), c);
        
        R_ShaderPackUploadInt(&v_solid_state_state.three_dim, str_lit("u_id"), i + 1);
        R_Draw(v_solid_state_state.objects[i], 0, v_solid_state_state.obj_v_count[i]);
    }
    
    R_PipelineBind(&v_solid_state_state.lines_vertex_array);
    R_ShaderPackUploadMat4(&v_solid_state_state.three_dim_lines, str_lit("u_view"), view);
    R_Draw(&v_solid_state_state.lines_vertex_array, 0, v_solid_state_state.line_v_count);
    
    R_Cull(CullFace_None);
	
    R_FramebufferBindScreen();
    R_BlendDisable();
    R_DepthDisable();
    R_Clear(BufferMask_Color);
    R_PipelineBind(&v_solid_state_state.fullscreen_vertex_array);
    R_Texture2DBindTo(&v_solid_state_state.fbo_color0, 1);
    R_ShaderPackUploadInt(&v_solid_state_state.two_dim, str_lit("u_tex"), 1);
    R_Draw(&v_solid_state_state.fullscreen_vertex_array, 0, 6);
}

dll_export void Render(R2D_Renderer* renderer) {
    UI_SetColorProperty(ColorProperty_Button_Base, (vec4) { 0.3f, 0.3f, 0.3f, 1.0f });
    UI_SetColorProperty(ColorProperty_Button_Hover, (vec4) { 0.2f, 0.2f, 0.2f, 1.0f });
    UI_SetColorProperty(ColorProperty_Button_Click, (vec4) { 0.35f, 0.35f, 0.35f, 1.0f });
    UI_SetFloatProperty(FloatProperty_Rounding, 20);
    
    if (UI_ButtonLabeled((rect) { 10, 10, 150, 40 }, str_lit("Simple Cubic"))) {
        SwitchScene(PackType_SimpleCubic);
    } else if (UI_ButtonLabeled((rect) { 10, 60, 150, 40 }, str_lit("FCC"))) {
        SwitchScene(PackType_FaceCentered);
    } else if (UI_ButtonLabeled((rect) { 10, 110, 150, 40 }, str_lit("BCC"))) {
        SwitchScene(PackType_BodyCentered);
    }
    
    UI_SetFloatProperty(FloatProperty_Rounding, 3);
    UI_SetColorProperty(ColorProperty_Slider_Base, (vec4) { 0.3f, 0.3f, 0.3f, 1.0f });
    UI_SetColorProperty(ColorProperty_Slider_BobBase, (vec4) { 0.5f, 0.5f, 0.5f, 1.0f });
    UI_SetColorProperty(ColorProperty_Slider_BobHover, (vec4) { 0.45f, 0.45f, 0.45f, 1.0f });
    UI_SetColorProperty(ColorProperty_Slider_BobDrag, (vec4) { 0.35f, 0.35f, 0.35f, 1.0f });
    
    UI_Slider((rect) { 10, 165, 150, 10 }, (vec2) { 10, 25 }, -20.f, 0.f, &data.cam.target_zoom);
    UI_Label((vec2) { 170, 175 }, str_lit("Zoom"));
    
    if (UI_Slider((rect) { 10, 190, 150, 10 }, (vec2) { 10, 25 }, v_solid_state_state.sphere_radius_range.x, v_solid_state_state.sphere_radius_range.y, &v_solid_state_state.sphere_radius)) {
        RefreshTransforms();
    }
	
	UI_Label((vec2) { 170, 200 }, str_lit("Radius"));
    
    UI_SetColorProperty(ColorProperty_Checkbox_Base, (vec4) { 0.3f, 0.3f, 0.3f, 1.0f });
    UI_SetColorProperty(ColorProperty_Checkbox_Hover, (vec4) { 0.45f, 0.45f, 0.45f, 1.0f });
    UI_SetColorProperty(ColorProperty_Checkbox_Inner, (vec4) { 0.65f, 0.65f, 0.65f, 1.0f });
    UI_SetFloatProperty(FloatProperty_Checkbox_Padding, 3.5f);
    
    if (UI_Checkbox((rect) { 10, 215, 25, 25 }, &v_solid_state_state.threexthree)) {
        Push3X3Lines();
        R_BufferUpdate(&v_solid_state_state.lines_buffer, 0, sizeof(LineVertex) * LINE_COUNT * 2, v_solid_state_state.lines_data);
    }
    UI_Label((vec2) { 45, 235 }, str_lit("3X3"));
}

dll_export void Update(f32 dt) {
    Camera3DControls(&data.cam, dt);
    
    f32 x = OS_InputGetMouseX();
    f32 y = OS_InputGetMouseY();
    y = v_solid_state_state.framebuffer_height - y;
    
    if (OS_InputButtonPressed(Input_MouseButton_Left)) {
        v_solid_state_state.pressed_loc_x = (u32) x;
        v_solid_state_state.pressed_loc_y = (u32) y;
    }
    if (OS_InputButtonReleased(Input_MouseButton_Left)) {
        if (v_solid_state_state.pressed_loc_x == x && v_solid_state_state.pressed_loc_y == y) {
            i32 v;
			R_FramebufferReadPixel(&v_solid_state_state.fbo, 1, (u32)x, (u32)y, &v);
            v_solid_state_state.selected_obj_id = v;
        }
    }
}

dll_export void OnResize(u32 new_w, u32 new_h) {
    data.projection = mat4_perspective(80, (f32)new_w / (f32)new_h, 1, 1000);
    if (new_w != 0 && new_h != 0)
        R_FramebufferResize(&v_solid_state_state.fbo, new_w, new_h);
    v_solid_state_state.framebuffer_width = new_w;
    v_solid_state_state.framebuffer_height = new_h;
}

dll_export void Free(void) {
    R_BufferFree(&v_solid_state_state.sphere_buffer);
    R_PipelineFree(&v_solid_state_state.sphere_vertex_array);
    R_BufferFree(&v_solid_state_state.fullscreen_buffer);
    R_PipelineFree(&v_solid_state_state.fullscreen_vertex_array);
    R_BufferFree(&v_solid_state_state.lines_buffer);
    R_PipelineFree(&v_solid_state_state.lines_vertex_array);
    
    R_ShaderPackFree(&v_solid_state_state.three_dim);
    R_ShaderPackFree(&v_solid_state_state.three_dim_lines);
    R_ShaderPackFree(&v_solid_state_state.two_dim);
    R_FramebufferFree(&v_solid_state_state.fbo);
    R_Texture2DFree(&v_solid_state_state.fbo_color0);
    R_Texture2DFree(&v_solid_state_state.fbo_color1);
    R_Texture2DFree(&v_solid_state_state.fbo_depth);
	
	string packed_data = {
		.str = (u8*) &data,
		.size = sizeof(solidstate_file),
	};
	OS_FileWrite(fp, packed_data);
	arena_free(&arena);
}
