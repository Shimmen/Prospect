#version 460

#include <common.glsl>

#include <shader_locations.h>
#include <camera_uniforms.h>

in vec2 v_tex_coord;
in vec3 v_position;

in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;

in vec4 v_curr_proj_pos;
in vec4 v_prev_proj_pos;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera;
};

uniform sampler2D u_base_color;
uniform sampler2D u_normal_map;
uniform sampler2D u_roughness_map;
uniform sampler2D u_metallic_map;

PredefinedOutput(vec4, o_g_buffer_albedo);
PredefinedOutput(vec4, o_g_buffer_material);
PredefinedOutput(vec4, o_g_buffer_norm_vel);

void main()
{
    o_g_buffer_albedo = texture(u_base_color, v_tex_coord);

    float roughness = texture(u_roughness_map, v_tex_coord).r;
    float metallic = texture(u_metallic_map, v_tex_coord).r;
    o_g_buffer_material = vec4(roughness, metallic, 1.0, 1.0);

    vec3 mapped_normal = unpackNormalMapNormal(texture(u_normal_map, v_tex_coord).xyz);
    mat3 tbn_matrix = createTbnMatrix(v_tangent, v_bitangent, v_normal);
    vec3 N = normalize(tbn_matrix * mapped_normal);

    vec2 curr01Pos = (v_curr_proj_pos.xy / v_curr_proj_pos.w) * 0.5 + 0.5;
    vec2 prev01Pos = (v_prev_proj_pos.xy / v_prev_proj_pos.w) * 0.5 + 0.5;
    vec2 screenSpaceVelocity = curr01Pos - prev01Pos;
    screenSpaceVelocity -= camera.frustum_jitter.xy;

    o_g_buffer_norm_vel = vec4(octahedralEncode(N), screenSpaceVelocity);
}
