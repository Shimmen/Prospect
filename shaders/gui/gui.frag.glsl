#version 460

#include <shader_locations.h>

in vec2 v_uv;
in vec4 v_color;

PredefinedUniform(sampler2D, u_gui_texture);

PredefinedOutput(vec4, o_color);

void main()
{
    o_color = v_color * texture(u_gui_texture, v_uv);
}
