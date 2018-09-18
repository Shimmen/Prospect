#version 460

#include <common.glsl>

#include <shader_locations.h>

in vec2 v_tex_coord;
in vec3 v_position;
in vec3 v_normal;

uniform sampler2D u_diffuse;

PredefinedOutput(vec4, o_g_buffer_albedo);
PredefinedOutput(vec4, o_g_buffer_material);
PredefinedOutput(vec4, o_g_buffer_normal);

void main()
{
    o_g_buffer_albedo = texture(u_diffuse, v_tex_coord);
    o_g_buffer_material = vec4(0.0, 1.0, 1.0, 1.0); // TODO
    o_g_buffer_normal = vec4(packNormal(v_normal), 1.0);
}
