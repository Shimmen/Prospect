#version 460

#include <brdf.glsl>
#include <common.glsl>
#include <shader_locations.h>
#include <camera_uniforms.h>

#include <shader_types.h>
#include <shader_constants.h>

in vec2 v_uv;
in vec3 v_view_ray;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

PredefinedUniformBlock(ShadowMapSegmentBlock)
{
    ShadowMapSegment shadowMapSegments[SHADOW_MAP_SEGMENT_MAX_COUNT];
};

PredefinedUniformBlock(DirectionalLightBlock)
{
    DirectionalLight directionalLight;
};

PredefinedUniform(sampler2D, u_g_buffer_albedo);
PredefinedUniform(sampler2D, u_g_buffer_material);
PredefinedUniform(sampler2D, u_g_buffer_norm_vel);
PredefinedUniform(sampler2D, u_g_buffer_depth);

PredefinedUniform(sampler2D, u_shadow_map);

PredefinedOutput(vec4, o_color);

float linearizeDepth(float nonLinearDepth)
{
    float projectionA = camera_uniforms.near_far.z;
    float projectionB = camera_uniforms.near_far.w;
    return projectionB / (nonLinearDepth - projectionA);
}

void main()
{
    float depth = texture(u_g_buffer_depth, v_uv).x;
    vec4 viewSpacePos = vec4(v_view_ray * linearizeDepth(depth), 1.0);

    vec2 packedNormal = texture(u_g_buffer_norm_vel, v_uv).xy;
    bool unlit = lengthSquared(packedNormal) < 0.0001;

    vec3 N = octahedralDecode(packedNormal);
    vec3 L = -normalize(directionalLight.viewDirecion.xyz);
    vec3 V = -normalize(viewSpacePos.xyz);
    float LdotN = max(dot(L, N), 0.0);

    float shadowFactor = 1.0;
    {
        int segmentIdx = directionalLight.shadowMapSegmentIndex.x;
        ShadowMapSegment segment = shadowMapSegments[segmentIdx];

        mat4 lightProjectionFromView = segment.lightViewProjection * camera_uniforms.world_from_view;
        vec4 posInShadowMap = lightProjectionFromView * viewSpacePos;
        posInShadowMap.xyz /= posInShadowMap.w;

        //float bias = 0.001 * pow(1.0 - LdotN, 0.5);
        float bias = 0.0006 - 0.0006 * pow(LdotN, 10.0);

        // Compare depths for shadows
        float actualDepth = posInShadowMap.z * 0.5 + 0.5;
        vec2 shadowMapUv = (segment.uvTransform * vec4(posInShadowMap.xy, 0.0, 1.0)).xy;
        float mapDepth = texture(u_shadow_map, shadowMapUv).x;
        shadowFactor = (mapDepth < actualDepth + bias) ? 0.0 : 1.0;

        // Fragments outside the shadow map should be considered to be in shadow
        // (Maybe not for large scale renders, but for smaller scenes I think this makes sense)
        bool insideBounds = posInShadowMap.xy == clamp(posInShadowMap.xy, vec2(-1.0), vec2(1.0));
        shadowFactor = (insideBounds) ? shadowFactor : 0.0;
    }

    vec3 lightColor = rgbFromColor(directionalLight.color);
    vec3 directLight = lightColor * shadowFactor;

    vec3 baseColor = texture(u_g_buffer_albedo, v_uv).rgb;
    vec4 material = texture(u_g_buffer_material, v_uv);
    float roughness = material.x + 0.01;
    float metallic = material.y;

    vec3 specular = specularBRDF(L, V, N, baseColor, roughness, metallic);
    // TODO: Add the multiscatter estimation from Filament 4.7.2:
    // vec3 energyCompensation = 1.0 + f0 * (1.0 / dfg.y - 1.0);
    // // Scale the specular lobe to account for multiscattering
    // Fr *= pixel.energyCompensation;

    vec3 diffuseColor = vec3(1.0 - metallic) * baseColor;
    vec3 diffuse = diffuseColor * diffuseBRDF();
    vec3 brdf = diffuse + specular;

    vec3 color = brdf * directLight * LdotN;

    color = (unlit) ? baseColor : color;
    o_color = vec4(color, 1.0);
}
