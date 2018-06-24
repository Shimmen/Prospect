#version 460

#include <shader_locations.h>
#include <camera_uniforms.h>

PredefinedAttribte(vec3, a_position);
PredefinedAttribte(vec2, a_tex_coord);

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

uniform mat4 u_world_from_local;

out vec2 v_tex_coord;
out vec3 v_position;

void main()
{
    v_tex_coord = a_tex_coord;

    vec4 world_space_position = u_world_from_local * vec4(a_position, 1.0);
    vec4 view_space_position  = camera_uniforms.view_from_world * world_space_position;

    v_position = view_space_position.xyz;

    gl_Position = camera_uniforms.projection_from_view * view_space_position;
}
