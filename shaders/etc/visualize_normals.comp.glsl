#version 460

#include <common.glsl>

layout(
    local_size_x = 32,
    local_size_y = 32
) in;

layout(binding = 0, rgba16f) restrict readonly  uniform image2D img_g_buffer_norm_vel;
layout(binding = 1, rgba8)   restrict writeonly uniform image2D img_normal_vis;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 packedNormal = imageLoad(img_g_buffer_norm_vel, pixelCoord).xy;

    if (lengthSquared(packedNormal) > 0.01)
    {
        vec3 N = octahedralDecode(packedNormal);
        vec4 color = vec4(N * 0.5 + 0.5, 1.0);
        imageStore(img_normal_vis, pixelCoord, color);
    }
    else
    {
        imageStore(img_normal_vis, pixelCoord, vec4(0.0, 0.0, 0.0, 1.0));
    }
}
