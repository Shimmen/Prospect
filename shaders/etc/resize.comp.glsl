#version 460

#include <common.glsl>
#include <brdf.glsl>

layout(
    local_size_x = 32,
    local_size_y = 32
) in;

uniform sampler2D u_source;
layout(binding = 0, rgba16f) restrict writeonly uniform image2D u_target;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (vec2(pixelCoord) + vec2(0.5)) / imageSize(u_target);
    vec4 color = texture(u_source, uv);
    imageStore(u_target, pixelCoord, color);
}
