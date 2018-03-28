#version 460

#include <mesh_attributes.h>

layout(location = MESH_ATTRIB_POSITION)  in vec3 a_vertex;
layout(location = MESH_ATTRIB_TEX_COORD) in vec2 a_uv;

out vec2 v_uv;

void main()
{
    v_uv = a_uv;
    gl_Position = vec4(a_vertex, 1.0);
}
