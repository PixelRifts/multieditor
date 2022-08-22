#version 330 core

in float v_texindex;
in vec2  v_texcoord;
in vec4  v_color;
in vec3  v_roundingparams;
in vec2  v_vertid;

layout (location = 0) out vec4 f_color;

uniform sampler2D u_tex[8];

const float smoothness = 0.25;

float round_corners() {
    vec2 pixelPos = v_vertid * vec2(v_roundingparams.x, v_roundingparams.y);
    vec2 minCorner = vec2(v_roundingparams.z, v_roundingparams.z);
    vec2 maxCorner = vec2(v_roundingparams.x - v_roundingparams.z, v_roundingparams.y - v_roundingparams.z);
    
    vec2 cornerPoint = clamp(pixelPos, minCorner, maxCorner);
    float lowerBound = (v_roundingparams.z - smoothness);
    float upperBound = (v_roundingparams.z + smoothness);
    
    float ppxmin = 1.0 - step(minCorner.x, pixelPos.x);
    float ppxmax = 1.0 - step(pixelPos.x, maxCorner.x);
    float ppymin = 1.0 - step(minCorner.y, pixelPos.y);
    float ppymax = 1.0 - step(pixelPos.y, maxCorner.y);
    
    float boolean = step(1, (ppxmin + ppxmax) * (ppymin + ppymax));
	float cornerAlpha = 1.0 - smoothstep(lowerBound, upperBound, distance(pixelPos, cornerPoint));
    return boolean * cornerAlpha + (1. - boolean);
}

void main() {
    switch (int(v_texindex)) {
        case 0: f_color = v_color * texture(u_tex[0], v_texcoord); break;
        case 1: f_color = v_color * texture(u_tex[1], v_texcoord); break;
        case 2: f_color = v_color * texture(u_tex[2], v_texcoord); break;
        case 3: f_color = v_color * texture(u_tex[3], v_texcoord); break;
        case 4: f_color = v_color * texture(u_tex[4], v_texcoord); break;
        case 5: f_color = v_color * texture(u_tex[5], v_texcoord); break;
        case 6: f_color = v_color * texture(u_tex[6], v_texcoord); break;
        case 7: f_color = v_color * texture(u_tex[7], v_texcoord); break;
        default: discard;
    }
	f_color.a *= round_corners();
}
