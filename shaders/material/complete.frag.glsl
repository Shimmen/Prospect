#version 460

#include <common.glsl>

#include <shader_locations.h>

in vec2 v_tex_coord;
in vec3 v_position;

in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;

uniform sampler2D u_base_color;
uniform sampler2D u_normal_map;
uniform sampler2D u_roughness_map;
uniform sampler2D u_metallic_map;

PredefinedOutput(vec4, o_g_buffer_albedo);
PredefinedOutput(vec4, o_g_buffer_material);
PredefinedOutput(vec4, o_g_buffer_normal);

void main()
{
    o_g_buffer_albedo = texture(u_base_color, v_tex_coord);

    float roughness = texture(u_roughness_map, v_tex_coord).r;
    float metallic = texture(u_metallic_map, v_tex_coord).r;
    o_g_buffer_material = vec4(roughness, metallic, 1.0, 1.0);

    vec3 mapped_normal = unpackNormal(texture(u_normal_map, v_tex_coord).xyz);
    mapped_normal.y *= -1.0; // (flip normal y to get correct up-axis)

    mat3 tbn_matrix = createTbnMatrix(v_tangent, v_bitangent, v_normal);
    vec3 N = tbn_matrix * mapped_normal;
    o_g_buffer_normal = vec4(packNormal(N), 1.0);
}
