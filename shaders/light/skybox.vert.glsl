#version 460

#include <shader_locations.h>
#include <camera_uniforms.h>

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

out vec3 v_view_ray;

void main()
{
    // Defines a full screen triangle with CW vertex ordering
    // Fix quad at max depth (i.e. make z/w == 1.0) so that the sky renders behind all geometry
    vec2 uv = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 1.0, 1.0);

    vec4 viewSpacePos = camera_uniforms.view_from_projection * gl_Position;
    vec4 worldSpaceDir = camera_uniforms.world_from_view * vec4(viewSpacePos.xyz / viewSpacePos.w, 0.0);
    v_view_ray = worldSpaceDir.xyz;
    v_view_ray.y = 1.0 - v_view_ray.y;
}
