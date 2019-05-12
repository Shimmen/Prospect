#version 460

#include <common.glsl>
#include <brdf.glsl>

layout(
    local_size_x = 1,
    local_size_y = 1
) in;

layout(binding = 0, rg16f) restrict writeonly uniform image2D u_brdf_map;

vec3 ggxBrdfApproximation(vec3 specularColor, float roughness, float NdotV)
{
    // From https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
    const vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;

    //return specularColor * AB.x + AB.y;
    return vec3(AB.x, AB.y, 0.0);
}

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = (vec2(pixelCoord) + vec2(0.5)) / vec2(imageSize(u_brdf_map));

    float NdotV = uv.x;
    float roughness = uv.y * uv.y;

    const vec3 N = vec3(0.0, 0.0, 1.0);
    vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);

    float A = 0.0;
    float B = 0.0;

    const uint NUM_SAMPLES = 1024;
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec2 xi = hammersley(i, NUM_SAMPLES);
        vec3 H = D_GGX_importanceSample(xi, N, roughness);
        vec3 L = reflect(-V, H);

        float LdotN = max(L.z, 0.0);

        if (LdotN > 0.0)
        {
            float NdotV = max(V.z, 0.0);
            float NdotL = max(L.z, 0.0);
            float NdotH = max(H.z, 0.0);
            float VdotH = max(dot(V, H), 0.0);

            float G = G_SmithIBL(NdotV, NdotL, roughness);
            float G_visibility = (G * VdotH) / (NdotH * NdotV);
            float F = pow(1.0 - VdotH, 5.0);

            A += (1.0 - F) * G_visibility;
            B += F * G_visibility;
        }
    }

    A /= float(NUM_SAMPLES);
    B /= float(NUM_SAMPLES);

    //vec2 color = ggxBrdfApproximation(vec3(1.0), uv.y, uv.x).rg;
    vec2 color = vec2(A, B);

    imageStore(u_brdf_map, pixelCoord, vec4(color, 0.0, 1.0));
}
