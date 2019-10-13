#version 460

#include <shader_locations.h>
#include <camera_uniforms.h>

PredefinedUniformBlock(CameraUniformBlock, camera);

out vec2 v_uv;
out vec3 v_view_ray;

vec3 getViewDirection(mat4 viewFromProj, mat4 worldFromView)
{
    vec4 viewSpacePos = viewFromProj * gl_Position;
    viewSpacePos.xyz /= viewSpacePos.w;
    return mat3(worldFromView) * viewSpacePos.xyz;
}

void main()
{
    // Defines a full screen triangle with CW vertex ordering
    // Fix quad at max depth (i.e. make z/w == 1.0) so that the sky renders behind all geometry
    vec2 uv = vec2(gl_VertexID & 2, (gl_VertexID << 1) & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 1.0, 1.0);

    v_uv = uv;
    v_view_ray = getViewDirection(camera.view_from_projection, camera.world_from_view);

    // TODO: Fix image loading y-axis!
    v_view_ray.y = 1.0 - v_view_ray.y;
}
