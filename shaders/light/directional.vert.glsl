#version 460

#include <shader_locations.h>
#include <camera_uniforms.h>

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

out vec2 v_uv;
out vec3 v_view_ray;

void main()
{
    // Defines a full screen triangle with CW vertex ordering
    v_uv = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);
    gl_Position = vec4(v_uv * 2.0 - 1.0, 0.0, 1.0);

    // Unproject the NDC-position to view space
    vec4 viewSpacePos = camera_uniforms.view_from_projection * gl_Position;
    viewSpacePos.xyz /= viewSpacePos.w;

    // Normalize the z coordinate since we are in view space and all go in the same direction
    viewSpacePos.xyz /= viewSpacePos.z;
    v_view_ray = viewSpacePos.xyz;
}
