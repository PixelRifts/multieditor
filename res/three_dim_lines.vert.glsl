#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec4 a_color;

out vec4 v_color;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_transform;

void main() {
    v_color = a_color;
    
    vec4 world_pos = u_transform * vec4(a_pos, 1.0);
    gl_Position = u_projection * u_view * world_pos;
}

