#version 460

#include <uniform_locations.h>

in vec2 v_uv;
in vec3 v_position;

uniform sampler2D u_diffuse;

out vec4 o_color;

void main()
{
    o_color = texture(u_diffuse, v_uv);
}
