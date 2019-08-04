#version 460

#include <common.glsl>
#include <shader_locations.h>
#include <camera_uniforms.h>

in vec2 v_uv;
in vec3 v_view_ray;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera;
};

PredefinedUniform(sampler2D, u_texture);

PredefinedOutput(vec4, o_color);
PredefinedOutput(vec4, o_g_buffer_norm_vel);

void main()
{
    // Sky color
    {
        vec3 viewRay = normalize(v_view_ray);
        vec2 sample_uv = sphericalUvFromDirection(viewRay);
        vec3 skyColor = texture(u_texture, sample_uv).rgb;
        o_color = vec4(skyColor, 1.0);
    }

    // Sky velocity (or rather camera velocity for background)
    {
        vec4 ndc = vec4(v_uv * 2.0 - 1.0, 1.0, 1.0);
        vec4 posWs = camera.world_from_view * camera.view_from_projection * ndc;
        posWs.xyz /= posWs.w;

        vec4 prevNdc = camera.prev_projection_from_world * vec4(posWs.xyz, 1.0);
        prevNdc.xyz /= prevNdc.w;
        vec2 prevUv = (prevNdc * 0.5 + 0.5).xy;

        vec2 screenSpaceVelocity = v_uv - prevUv;
        screenSpaceVelocity -= camera.frustum_jitter.xy;
        o_g_buffer_norm_vel = vec4(0.0, 0.0, screenSpaceVelocity);
    }
}
