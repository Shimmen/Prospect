#version 460

#include <common.glsl>
#include <brdf.glsl>

#include <shader_constants.h>

layout(
    local_size_x = IBL_RADIANCE_OPT_LOCAL_SIZE,
    local_size_y = IBL_RADIANCE_OPT_LOCAL_SIZE
) in;

uniform float u_roughness;
uniform sampler2D u_radiance;

layout(binding = 0, rgba16f) restrict writeonly uniform image2D u_filtered_radiance;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (vec2(pixelCoord) + vec2(0.5)) / imageSize(u_filtered_radiance);

    vec3 N = directionFromSphericalUv(uv);

    // Assume R=V=N (the UE approximation)
    vec3 R = N;
    vec3 V = N;

    // Use 'a' for a pereceptually linear rougness
    float a = square(u_roughness);

    vec3 color = vec3(0.0);

    const uint NUM_SAMPLES = 4096;
    float totalWeight = 0.0;

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 xi = hammersley(i, NUM_SAMPLES);
        vec3 H = D_GGX_importanceSample(xi, N, a);

        vec3 L = reflect(-V, H);
        float LdotN = max(dot(L, N), 0.0);

        if (LdotN > 0.0)
        {
            float NdotH = dot(N, H);
            float HdotV = dot(H, V);

            float D = D_GGX(NdotH, a);
            float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001;

            // TODO: I feel as this is wrong since we currently aren't using cubemaps, but it sort of looks okay..?
            // In the near future we will be using cubemaps anyway (when we render the environment to it, so it'll be okay)
            const float resolution = 1024.0; // resolution of source map at level 0
            const float saTexel  = 4.0 * PI / (6.0 * square(resolution));
            float saSample = 1.0 / (float(NUM_SAMPLES) * pdf + 0.0001);

            float mipLevel = a < 0.001 ? 0.0 : 0.5 * log2(saSample / saTexel);

            vec2 sourceUv = sphericalUvFromDirection(L);
            vec3 impSampled = textureLod(u_radiance, sourceUv, mipLevel).rgb * LdotN;

            color += impSampled;
            totalWeight += LdotN;
        }
    }

    color /= totalWeight;

    imageStore(u_filtered_radiance, pixelCoord, vec4(color, 1.0));
}
