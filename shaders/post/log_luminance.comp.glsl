#version 460

#include <common.glsl>
#include <shader_locations.h>

layout(
    local_size_x = 32,
    local_size_y = 32
) in;


PredefinedUniform(sampler2D, u_texture);
layout(binding = 1, r32f) restrict writeonly uniform image2D img_target;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (vec2(pixelCoord) + vec2(0.5)) / imageSize(img_target);

    vec3 color = texture(u_texture, uv).rgb;

    float lum = luminance(color);
    const float epsilon = 0.0001;
    float logLum = log(lum + epsilon);

    imageStore(img_target, pixelCoord, vec4(vec3(logLum), 1.0));
}
