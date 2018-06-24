#version 460

#include <mesh_attributes.h>
#include <shader_locations.h>

layout(location = MESH_ATTRIB_POSITION)  in vec3 a_vertex;
layout(location = MESH_ATTRIB_TEX_COORD) in vec2 a_uv;

PredefinedUniform(mat4, u_view_from_world);
PredefinedUniform(mat4, u_projection_from_view);

uniform mat4 u_world_from_local;

out vec2 v_uv;
out vec3 v_position;

void main()
{
    v_uv = a_uv;

    vec4 world_space_position = u_world_from_local * vec4(a_vertex, 1.0);
    vec4 view_space_position  = u_view_from_world * world_space_position;

    v_position = view_space_position.xyz;

    gl_Position = u_projection_from_view * view_space_position;
}
