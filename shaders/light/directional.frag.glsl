#version 460

#include <shader_locations.h>

in vec2 v_uv;
in vec3 v_position;

PredefinedUniform(vec3, u_directional_light_color);
PredefinedUniform(vec3, u_directional_light_direction);

uniform vec3 u_direction;
uniform vec4 u_color;
uniform float u_ambient;

out vec4 o_color;

void main()
{
    o_color = texture(u_diffuse, v_uv);
}
