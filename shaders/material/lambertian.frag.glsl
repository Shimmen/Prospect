#version 460

#include <shader_locations.h>

in vec2 v_tex_coord;
in vec3 v_position;
in vec3 v_normal;

uniform sampler2D u_diffuse;

PredefinedOutput(vec4, o_g_buffer_albedo);
PredefinedOutput(vec4, o_g_buffer_normal);

void main()
{
    o_g_buffer_albedo = texture(u_diffuse, v_tex_coord);
    o_g_buffer_normal = vec4(v_normal * 0.5 + 0.5, 1.0);
}
