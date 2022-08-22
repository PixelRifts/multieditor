#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;

out vec3 v_to_light;
out vec3 v_normal;
out vec3 v_to_cam;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_transform;

vec3 light_pos = vec3(0.0, 10.0, 0.0);

void main() {
    vec4 world_pos = u_transform * vec4(a_pos, 1.0);
    
    vec3 to_light = light_pos - a_pos;
    v_normal = (u_transform * vec4(a_normal, 0.0)).xyz;
    v_to_light = light_pos - world_pos.xyz;
    v_to_cam = (inverse(u_view) * vec4(0.0, 0.0, 0.0, 1.0)).xyz - world_pos.xyz;
    
    gl_Position = u_projection * u_view * world_pos;
}
