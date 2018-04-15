#version 460

#include <mesh_attributes.h>

layout(location = MESH_ATTRIB_POSITION)  in vec3 a_vertex;
layout(location = MESH_ATTRIB_TEX_COORD) in vec2 a_uv;

uniform mat4 u_world_from_local;

out vec2 v_uv;

void main()
{
    v_uv = a_uv;

    gl_Position = u_world_from_local * vec4(a_vertex, 1.0);
}
