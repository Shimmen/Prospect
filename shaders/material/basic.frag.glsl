#version 460

#include <common.glsl>

#include <shader_locations.h>

in vec2 v_tex_coord;
in vec3 v_position;
in vec3 v_normal;

uniform vec3 u_base_color;
uniform float u_roughness;
uniform float u_metallic;

PredefinedOutput(vec4, o_g_buffer_albedo);
PredefinedOutput(vec4, o_g_buffer_material);
PredefinedOutput(vec4, o_g_buffer_normal);

void main()
{
    o_g_buffer_albedo = vec4(u_base_color, 1.0);
    o_g_buffer_material = vec4(u_roughness, u_metallic, 1.0, 1.0);
    o_g_buffer_normal = vec4(packNormal(v_normal), 1.0);
}
