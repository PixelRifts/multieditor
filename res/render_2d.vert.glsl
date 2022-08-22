#version 330 core

layout (location = 0) in vec2  a_pos;
layout (location = 1) in vec2  a_texcoord;
layout (location = 2) in float a_texindex;
layout (location = 3) in vec4  a_color;
layout (location = 4) in vec3  a_roundingparams;
layout (location = 5) in vec2  a_vertid;

out float v_texindex;
out vec2  v_texcoord;
out vec4  v_color;
out vec3  v_roundingparams;
out vec2  v_vertid;

uniform mat4 u_projection;

void main() {
    gl_Position = u_projection * vec4(a_pos, 0.0, 1.0);
    v_texindex = a_texindex;
    v_texcoord = a_texcoord;
    v_color = a_color;
	v_roundingparams = a_roundingparams;
	v_vertid = a_vertid;
}
