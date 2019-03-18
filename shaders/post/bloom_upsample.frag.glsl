#version 460

#include <shader_locations.h>

in vec2 v_uv;

PredefinedUniform(sampler2D, u_texture);
uniform sampler2D u_texture_to_blur;
uniform float u_texel_aspect;
uniform float u_blur_radius;
uniform int u_target_lod;

PredefinedOutput(vec4, o_color);

vec4 tent3x3Upsample(vec2 uv, vec2 offset, float sampleLod)
{
    vec2 off = offset;

    vec4 color = 6.0 * textureLod(u_texture_to_blur, uv, sampleLod);

    color += 2.0 * textureLod(u_texture_to_blur, uv + vec2(-off.x, 0.0), sampleLod);
    color += 2.0 * textureLod(u_texture_to_blur, uv + vec2(+off.x, 0.0), sampleLod);
    color += 2.0 * textureLod(u_texture_to_blur, uv + vec2(0.0, -off.y), sampleLod);
    color += 2.0 * textureLod(u_texture_to_blur, uv + vec2(0.0, +off.y), sampleLod);

    color += 1.0 * textureLod(u_texture_to_blur, uv + vec2(-off.x, -off.y), sampleLod);
    color += 1.0 * textureLod(u_texture_to_blur, uv + vec2(-off.x, +off.y), sampleLod);
    color += 1.0 * textureLod(u_texture_to_blur, uv + vec2(+off.x, -off.y), sampleLod);
    color += 1.0 * textureLod(u_texture_to_blur, uv + vec2(+off.x, +off.y), sampleLod);

    return color / vec4(18.0);
}

void main()
{
    float sampleLod = float(u_target_lod + 1);
    vec2 offset = vec2(1.0, u_texel_aspect) * vec2(u_blur_radius);
    vec4 blurred = tent3x3Upsample(v_uv, offset, sampleLod);

    vec4 current = textureLod(u_texture, v_uv, float(u_target_lod));

    o_color = current + blurred;
}
