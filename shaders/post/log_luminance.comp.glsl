#version 460

#include <common.glsl>

layout(
    local_size_x = 256,
    local_size_y = 256
) in;

layout(binding = 0, rgba16f) restrict readonly  uniform image2D img_source;
layout(binding = 1, r16f)    restrict writeonly uniform image2D img_target;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec3 color = imageRead(img_source, pixelCoord).rgb;

    float lum = luminance(color);
    float logLum = log2(lum);

    imageStore(img_target, pixelCoord, logLum);
}
