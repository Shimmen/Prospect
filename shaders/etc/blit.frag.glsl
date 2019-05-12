#version 460

#include <shader_locations.h>

in vec2 v_uv;

PredefinedUniform(sampler2D, u_texture);

PredefinedOutput(vec4, o_color);

void main()
{
    o_color = texture(u_texture, v_uv);
}
