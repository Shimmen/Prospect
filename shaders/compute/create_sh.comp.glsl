#version 460

#include <common.glsl>
#include <brdf.glsl>

#include <shader_constants.h>
#include <shader_locations.h>

layout(
    local_size_x = 3,
    local_size_y = 3
) in;

PredefinedUniformBlock(SphereSampleBuffer)
{
    // a-component unused!
    vec4 u_samples[SPHERE_SAMPLES_COUNT];
};

uniform sampler2D u_radiance;

layout(binding = 0, rgba16f) restrict uniform image2D u_sh_coefficients;

void main()
{
    for (int i = 0; i < SPHERE_SAMPLES_COUNT; ++i)
    {
        vec3 N = normalize(u_samples[i].xyz);
        vec2 sampleUv = sphericalUvFromDirection(N);
        vec3 L = texture(u_radiance, sampleUv).rgb;

        float Y00     = 0.282095;
        float Y11     = 0.488603 * N.x;
        float Y10     = 0.488603 * N.z;
        float Y1_1    = 0.488603 * N.y;
        float Y21     = 1.092548 * N.x * N.z;
        float Y2_1    = 1.092548 * N.y * N.z;
        float Y2_2    = 1.092548 * N.y * N.x;
        float Y20     = 0.946176 * N.z * N.z - 0.315392;
        float Y22     = 0.546274 * (N.x * N.x - N.y * N.y);

        vec3 L00   = L * Y00  / float(SPHERE_SAMPLES_COUNT);
        vec3 L11   = L * Y11  / float(SPHERE_SAMPLES_COUNT);
        vec3 L10   = L * Y10  / float(SPHERE_SAMPLES_COUNT);
        vec3 L1_1  = L * Y1_1 / float(SPHERE_SAMPLES_COUNT);
        vec3 L21   = L * Y21  / float(SPHERE_SAMPLES_COUNT);
        vec3 L2_1  = L * Y2_1 / float(SPHERE_SAMPLES_COUNT);
        vec3 L2_2  = L * Y2_2 / float(SPHERE_SAMPLES_COUNT);
        vec3 L20   = L * Y20  / float(SPHERE_SAMPLES_COUNT);
        vec3 L22   = L * Y22  / float(SPHERE_SAMPLES_COUNT);

        imageStore(u_sh_coefficients, ivec2(0,0), imageLoad(u_sh_coefficients, ivec2(0,0)) + vec4(L00,  0.0));
        imageStore(u_sh_coefficients, ivec2(1,0), imageLoad(u_sh_coefficients, ivec2(1,0)) + vec4(L11,  0.0));
        imageStore(u_sh_coefficients, ivec2(2,0), imageLoad(u_sh_coefficients, ivec2(2,0)) + vec4(L10,  0.0));
        imageStore(u_sh_coefficients, ivec2(0,1), imageLoad(u_sh_coefficients, ivec2(0,1)) + vec4(L1_1, 0.0));
        imageStore(u_sh_coefficients, ivec2(1,1), imageLoad(u_sh_coefficients, ivec2(1,1)) + vec4(L21,  0.0));
        imageStore(u_sh_coefficients, ivec2(2,1), imageLoad(u_sh_coefficients, ivec2(2,1)) + vec4(L2_1, 0.0));
        imageStore(u_sh_coefficients, ivec2(0,2), imageLoad(u_sh_coefficients, ivec2(0,2)) + vec4(L2_2, 0.0));
        imageStore(u_sh_coefficients, ivec2(1,2), imageLoad(u_sh_coefficients, ivec2(1,2)) + vec4(L20,  0.0));
        imageStore(u_sh_coefficients, ivec2(2,2), imageLoad(u_sh_coefficients, ivec2(2,2)) + vec4(L22,  0.0));

        memoryBarrierImage();
    }
}
