#version 460

#include <shader_locations.h>
#include <camera_uniforms.h>

PredefinedUniformBlock(CameraUniformBlock, camera);

out vec2 v_uv;
out vec3 v_view_ray;

void main()
{
    // Defines a full screen triangle with CW vertex ordering
    v_uv = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);
    gl_Position = vec4(v_uv * 2.0 - 1.0, 0.0, 1.0);

    // Unproject the NDC-position to view space
    vec4 viewSpacePos = camera.view_from_projection * gl_Position;
    viewSpacePos.xyz /= viewSpacePos.w;

    // Normalize the z coordinate since we interpolate from a 2D/xy-quad
    // (i.e. there is no relevant z-component to consider when interpolating)
    viewSpacePos.xyz /= viewSpacePos.z;
    v_view_ray = viewSpacePos.xyz;
}
