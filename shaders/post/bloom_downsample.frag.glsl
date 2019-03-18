#version 460

#include <shader_locations.h>

in vec2 v_uv;

PredefinedUniform(sampler2D, u_texture);
uniform vec2 u_target_texel_size;
uniform int  u_target_lod;

PredefinedOutput(vec4, o_color);

// Assure bilinear filtering of u_texture and clamp to edge! For reference see:
// https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
vec4 codCustomDownsample(vec2 uv, vec2 targetTexelSize, float sampleLod)
{
    vec4 color = vec4(0.0);

    vec2 off = targetTexelSize;
    vec2 halfOff = off / 2.0;

    // Center 4x4 box, weight 0.5
    color += (0.5 / 4.0) * textureLod(u_texture, uv + vec2(-halfOff.x, -halfOff.y), sampleLod);
    color += (0.5 / 4.0) * textureLod(u_texture, uv + vec2(-halfOff.x, +halfOff.y), sampleLod);
    color += (0.5 / 4.0) * textureLod(u_texture, uv + vec2(+halfOff.x, -halfOff.y), sampleLod);
    color += (0.5 / 4.0) * textureLod(u_texture, uv + vec2(+halfOff.x, +halfOff.y), sampleLod);

    // Top-left & top-right & bottom-left & bottom right samples (not shared), weight 0.125
    color += (0.125 / 4.0) * textureLod(u_texture, uv + vec2(-off.x, -off.y), sampleLod);
    color += (0.125 / 4.0) * textureLod(u_texture, uv + vec2(-off.x, +off.y), sampleLod);
    color += (0.125 / 4.0) * textureLod(u_texture, uv + vec2(+off.x, -off.y), sampleLod);
    color += (0.125 / 4.0) * textureLod(u_texture, uv + vec2(+off.x, +off.y), sampleLod);

    // Centered "plus sign", where every sample is shared by two 4x4 boxes, weight 0.125
    color += (0.125 / 2.0) * textureLod(u_texture, uv + vec2(-off.x, 0.0), sampleLod);
    color += (0.125 / 2.0) * textureLod(u_texture, uv + vec2(+off.x, 0.0), sampleLod);
    color += (0.125 / 2.0) * textureLod(u_texture, uv + vec2(0.0, -off.y), sampleLod);
    color += (0.125 / 2.0) * textureLod(u_texture, uv + vec2(0.0, +off.y), sampleLod);

    // Center sample, shared by all four offset 4x4 boxes, weight 0.125
    // (divide by 4 to get each sample in the 4x4, but then multiply with 4 since the overlap)
    color += 0.125 * textureLod(u_texture, uv, sampleLod);

    return color;
}

void main()
{
    // TODO: Consider if we should use target uv or source uv offsets
    // in the case of NPOT mip maps (which is pretty much always...)
    float sampleLod = float(u_target_lod - 1);
    o_color = codCustomDownsample(v_uv, u_target_texel_size, sampleLod);
}
