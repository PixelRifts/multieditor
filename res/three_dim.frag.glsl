#version 330 core

in vec3 v_to_light;
in vec3 v_normal;
in vec3 v_to_cam;

layout (location = 0) out vec4 f_color;
layout (location = 1) out int f_id;

const float damping = 10;
const float reflectivity = 0.5;
const float min_light = 0.2;

uniform vec4 u_color;
uniform int u_id;

float smax(float a, float b, float k) {
    return log(exp(k * a) + exp(k * b)) / k;
}

void main() {
    vec3 unit_normal = normalize(v_normal);
    vec3 unit_to_light = normalize(v_to_light);
    float diffuse_coefficient = smax(min_light, ((dot(unit_to_light, unit_normal) + 1) / 2), 1.2);
    vec3 diffuse = vec3(u_color.rgb * diffuse_coefficient);
    
    vec3 unit_to_cam = normalize(v_to_cam);
    vec3 unit_from_light = -unit_to_light;
    vec3 reflected = reflect(unit_from_light, unit_normal);
    float specular_coefficient = ((dot(reflected, unit_to_cam) + 1) / 2);
    vec3 specular = vec3(1.0) * pow(specular_coefficient, damping) * reflectivity;
    
    f_color = vec4(diffuse + specular, u_color.a);
    f_id = u_id;
}
