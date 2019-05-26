#version 460

#include <common.glsl>

layout(
    local_size_x = 32,
    local_size_y = 32
) in;

uniform float u_velocity_scale = 75.0;

layout(binding = 0, rgba16f) restrict readonly  uniform image2D img_g_buffer_norm_vel;
layout(binding = 1, rgba8)   restrict writeonly uniform image2D img_velocity_vis;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);

    vec2 velocity = imageLoad(img_g_buffer_norm_vel, pixelCoord).zw;

    float speed = length(velocity) * u_velocity_scale;
    float mappedSpeed = speed / (1.0 + speed);

    float direction = atan(velocity.y, velocity.x) / TWO_PI;
    vec3 dirColor = hsv2rgb(vec3(direction, 1.0, 1.0));

    vec3 velocityVis = dirColor * mappedSpeed;

    imageStore(img_velocity_vis, pixelCoord, vec4(velocityVis, 1.0));
}
