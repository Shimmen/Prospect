#version 460

#include <shader_locations.h>
#include <camera_uniforms.h>

PredefinedAttribute(vec3, a_position);
PredefinedAttribute(vec3, a_normal);
PredefinedAttribute(vec2, a_tex_coord);

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

uniform mat4 u_world_from_local;
uniform mat4 u_prev_world_from_local;
uniform mat3 u_world_from_tangent;

out vec2 v_tex_coord;
out vec3 v_position;
out vec3 v_normal;

out vec4 v_curr_proj_pos;
out vec4 v_prev_proj_pos;

void main()
{
    v_tex_coord = a_tex_coord;

    vec4 world_space_position = u_world_from_local * vec4(a_position, 1.0);
    vec4 view_space_position  = camera_uniforms.view_from_world * world_space_position;

    vec3 world_space_normal = u_world_from_tangent * a_normal;
    vec3 view_space_normal = mat3(camera_uniforms.view_from_world) * world_space_normal;

    v_position = view_space_position.xyz;
    v_normal = view_space_normal;

    gl_Position = camera_uniforms.projection_from_view * view_space_position;

    v_curr_proj_pos = gl_Position;
    v_prev_proj_pos = camera_uniforms.prev_projection_from_world * u_prev_world_from_local * vec4(a_position, 1.0);
}
