#version 460

#include <shader_locations.h>
#include <camera_uniforms.h>

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

out vec2 v_uv;

void main()
{
    // Defines a full screen triangle with CW vertex ordering
    v_uv = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);
    gl_Position = vec4(v_uv * 2.0 - 1.0, 0.0, 1.0);
}
