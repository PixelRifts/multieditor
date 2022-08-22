#version 330 core

in vec2 v_uv;
in vec4 v_color;

layout (location = 0) out vec4 f_color;

uniform sampler2D u_tex;

void main() {
    f_color = texture(u_tex, v_uv) * v_color;
}
