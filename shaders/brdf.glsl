#ifndef BRDF_GLSL
#define BRDF_GLSL

#include <common.glsl>

#define DIELECTRIC_REFLECTANCE (0.04)

//
// The current BRDF model in use is based on the one from Filament,
// documented here: https://google.github.io/filament/Filament.md#toc4.
// It's licenced under the Apache License 2.0 which can be found
// here: https://github.com/google/filament/blob/master/LICENSE.
//

float D_GGX(float NdotH, float a) {
    float a2 = a * a;
    float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
    float x = a2 / (PI * f * f + 1e-20);
    return x;
}

vec3 F_Schlick(float VdotH, vec3 f0) {
    return f0 + (vec3(1.0) - f0) * pow(1.0 - VdotH, 5.0);
}

float V_SmithGGXCorrelated(float NdotV, float NdotL, float a) {
    float a2 = a * a;
    float GGXL = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
    float GGXV = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
    return 0.5 / (GGXV + GGXL);
}

//

// For when there is no single H-vector to "choose" from (such as for IBL)
// From https://seblagarde.wordpress.com/2011/08/17/hello-world/
vec3 F_SchlickRoughnessCompensating(float VdotN, vec3 f0, float roughness)
{
    vec3 gloss = vec3(1.0 - roughness);
    return f0 + (max(gloss, f0) - f0) * pow(1.0 - VdotN, 5.0);
}

// For baked BRDF in the IBL calculations
float G_ShlickGGX(float NdotV, float roughness)
{
    float a = roughness; // should it really be roughness squared?!
    float k = (a * a) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_SmithIBL(float NdotV, float NdotL, float roughness)
{
    float GGXV = G_ShlickGGX(NdotV, roughness);
    float GGXL = G_ShlickGGX(NdotL, roughness);
    return GGXV * GGXL;
}

//

vec3 specularBRDF(vec3 L, vec3 V, vec3 N, vec3 baseColor, float roughness, float metallic)
{
    vec3 H = normalize(L + V);

    float NdotV = abs(dot(N, V)) + 1e-5;
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float LdotH = clamp(dot(L, H), 0.0, 1.0);

    // Use a which is perceptually linear for roughness
    float a = square(roughness);

    vec3 f0 = mix(vec3(DIELECTRIC_REFLECTANCE), baseColor, metallic);

    vec3 F = F_Schlick(LdotH, f0);
    float D = D_GGX(NdotH, a);
    float V_part = V_SmithGGXCorrelated(NdotV, NdotL, a);

    return F * vec3(D * V_part);
}

vec3 diffuseBRDF()
{
    return vec3(1.0) / vec3(PI);
}

//

vec3 D_GGX_importanceSample(vec2 xi, vec3 N, float a)
{
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVector = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVector);
}


#endif // BRDF_GLSL
