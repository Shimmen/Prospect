#version 460

#include <shader_locations.h>

PredefinedAttribte(vec3, a_position);

PredefinedUniform(mat4, u_projection_from_world);
PredefinedUniform(mat4, u_world_from_local);

void main()
{
    vec4 world_space_position = u_world_from_local * vec4(a_position, 1.0);
    gl_Position = u_projection_from_world * world_space_position;
}
