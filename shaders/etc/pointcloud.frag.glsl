#version 460

#include <shader_locations.h>

in vec3 v_color;
in float v_distance_from_eye;

PredefinedOutput(vec4, o_color);

void main()
{
    float alpha = 0.25 + 0.01 * v_distance_from_eye;
    alpha = clamp(alpha, 0.0, 1.0);
    o_color = vec4(v_color, alpha);
}
