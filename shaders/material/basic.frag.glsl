#version 460

#include <common.glsl>

#include <shader_locations.h>
#include <camera_uniforms.h>

in vec2 v_tex_coord;
in vec3 v_position;
in vec3 v_normal;

in vec4 v_curr_proj_pos;
in vec4 v_prev_proj_pos;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera;
};

uniform vec3 u_base_color;
uniform float u_roughness;
uniform float u_metallic;

PredefinedOutput(vec4, o_g_buffer_albedo);
PredefinedOutput(vec4, o_g_buffer_material);
PredefinedOutput(vec4, o_g_buffer_norm_vel);

void main()
{
    o_g_buffer_albedo = vec4(u_base_color, 1.0);
    o_g_buffer_material = vec4(u_roughness, u_metallic, 1.0, 1.0);

    vec2 curr01Pos = (v_curr_proj_pos.xy / v_curr_proj_pos.w) * 0.5 + 0.5;
    vec2 prev01Pos = (v_prev_proj_pos.xy / v_prev_proj_pos.w) * 0.5 + 0.5;
    vec2 screenSpaceVelocity = curr01Pos - prev01Pos;
    screenSpaceVelocity -= camera.frustum_jitter.xy;

    vec3 normal = normalize(v_normal);
    o_g_buffer_norm_vel = vec4(octahedralEncode(normal), screenSpaceVelocity);
}
