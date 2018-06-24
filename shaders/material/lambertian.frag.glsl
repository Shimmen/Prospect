#version 460

#include <shader_locations.h>

in vec2 v_uv;
in vec3 v_position;

uniform sampler2D u_diffuse;

PredefinedOutput(vec4, o_g_buffer_albedo);
PredefinedOutput(vec4, o_g_buffer_normal);

void main()
{
    o_g_buffer_albedo = texture(u_diffuse, v_uv);
    o_g_buffer_normal = vec4(1.0, 0.0, 1.0, 1.0);
}
