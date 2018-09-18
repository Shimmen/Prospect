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
    return a2 / (PI * f * f);
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

// roughness should be perceptually linear (i.e. squared)!
vec3 specularBRDF(vec3 L, vec3 V, vec3 N, vec3 baseColor, float roughness, float metallic)
{
    vec3 H = normalize(L + V);

    float NdotV = abs(dot(N, V)) + 1e-5;
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float LdotH = clamp(dot(L, H), 0.0, 1.0);

    vec3 f0 = mix(vec3(DIELECTRIC_REFLECTANCE), baseColor, metallic);

    vec3  F = F_Schlick(LoH, f0);
    float D = D_GGX(NoH, roughness);
    float V = V_SmithGGXCorrelated(NoV, NoL, roughness);

    return F * vec3(D * V);
}


vec3 diffuseBRDF()
{
    return vec3(1.0) / vec3(PI);
}

#endif // BRDF_GLSL
