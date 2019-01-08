#version 460

#include <common.glsl>
#include <shader_locations.h>

layout(
    local_size_x = 32,
    local_size_y = 32
) in;

PredefinedUniform(sampler2D, u_texture);
uniform float u_threshold;

layout(binding = 0, rgba16f) restrict writeonly uniform image2D u_target;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (vec2(pixelCoord) + vec2(0.5)) / imageSize(u_target);

    vec3 color = texture(u_texture, uv).rgb;
    float luminance = luminance(color);

    //vec3 outColor = (luminance > u_threshold) ? color : vec3(0.0);

    // This softness trick doesn't really make a difference, we still get the horrible aliasing
    float softness = 10.0;
    float softThreshold = smoothstep(u_threshold - softness, u_threshold + softness, luminance);
    softThreshold = pow(softThreshold, 1.0);
    vec3 outColor = mix(vec3(0.0), color, softThreshold);

    imageStore(u_target, pixelCoord, vec4(outColor, 1.0));
}
