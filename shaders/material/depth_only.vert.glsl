#version 460

#include <shader_locations.h>
#include <camera_uniforms.h>

PredefinedAttribute(vec3, a_position);

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

uniform mat4 u_world_from_local;

void main()
{
    vec4 world_space_position = u_world_from_local * vec4(a_position, 1.0);
    vec4 view_space_position  = camera_uniforms.view_from_world * world_space_position;
    gl_Position = camera_uniforms.projection_from_view * view_space_position;
}
