#version 460

#include <common.glsl>
#include <shader_locations.h>
#include <camera_uniforms.h>

in vec3 v_view_ray;

PredefinedUniform(sampler2D, u_texture);

PredefinedOutput(vec4, o_color);

void main()
{
    vec3 viewRay = normalize(v_view_ray);
    vec2 uv = sphericalUvFromDirection(viewRay);

    // TODO: Implement blending between left/right edge in sphere map!
    vec3 skyColor = texture(u_texture, uv).rgb;
    o_color = vec4(skyColor, 1.0);
}