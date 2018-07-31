#version 460

#include <shader_locations.h>

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec4 a_color;

PredefinedUniform(mat4, u_gui_projection);

out vec2 v_uv;
out vec4 v_color;

void main()
{
    v_uv = a_uv;
    v_color = a_color;

    gl_Position = u_gui_projection * vec4(a_position, 0.0, 1.0);
}
