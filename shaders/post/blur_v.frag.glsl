#version 460

#include <shader_locations.h>

in vec2 v_uv;

PredefinedUniform(sampler2D, u_texture);
uniform float u_texture_size;
uniform float u_texture_lod;

PredefinedOutput(vec4, o_color);

const float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);
const float weight[3] = float[](0.2270270270, 0.3162162162, 0.0702702703);

// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
// for more information on the implementation
void main()
{
    o_color = textureLod(u_texture, v_uv, u_texture_lod) * weight[0];

    for (int i = 1; i < 3; ++i)
    {
        vec2 offset = vec2(0.0, offset[i] / u_texture_size);
        o_color += textureLod(u_texture, v_uv + offset, u_texture_lod) * weight[i];
        o_color += textureLod(u_texture, v_uv - offset, u_texture_lod) * weight[i];
    }
}
