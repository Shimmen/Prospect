#version 460

#include <mesh_attributes.glsl>

out vec2 v_uv;

void main()
{
    v_uv = a_uv;
    gl_Position = vec4(a_vertex, 1.0);
}
