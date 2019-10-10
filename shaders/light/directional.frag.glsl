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

// 9 samples in a Fibbonaci spiral with radius 1.0f
const int numFibShadowSamples = 9;
const vec2 fibShadowSamples[] = vec2[numFibShadowSamples](
    vec2(0.0, 0.0),
    vec2(-0.2457896260261067, -0.22516343142050776),
    vec2(0.041212881865007774, 0.46959953214478733),
    vec2(0.3512823401715744, -0.45818560738735076),
    vec2(-0.6564756568769521, 0.11612130025287623),
    vec2(0.6288980651919658, 0.40005347036784666),
    vec2(-0.2119660273461883, -0.7885030563781865),
    vec2(-0.4064817883047546, 0.7826559483926168),
    vec2(0.8856006111249612, -0.3234199228000402)
);

float calculateShadowFactor(vec4 viewSpacePos, float LdotN)
{
    int segmentIdx = directionalLight.shadowMapSegmentIndex.x;
    ShadowMapSegment segment = shadowMapSegments[segmentIdx];
    vec2 shadowTexelSize = vec2(1.0) / vec2(textureSize(u_shadow_map, 0));

    const float fibRadius = 3.0; // TODO: Adjust/make parameter!
    float bias = 0.0006 - 0.0006 * pow(LdotN, 10.0);
    mat4 lightProjectionFromView = segment.lightViewProjection * camera_uniforms.world_from_view;

    // Generate rotation from a blue-noise value
    ivec2 noiseCoord = ivec2(gl_FragCoord.xy);// % ivec2(64);
    //float noise = fract(float(uFrameCount + 1) * goldenRatio * texelFetch(uBlueNoise, noiseCoord, 0).r);
    float noise = fract(122359.384549 * float(noiseCoord.x) + 239223.2435894 * float(noiseCoord.y));
    float rot = 6.283185 * noise;
    mat2 sampleRot = mat2(cos(rot), sin(rot), -sin(rot), cos(rot));

    float shadowAcc = 0.0;
    for (int i = 0; i < numFibShadowSamples; ++i)
    {
        vec4 posInShadowMap = lightProjectionFromView * viewSpacePos;
        posInShadowMap.xyz /= posInShadowMap.w;

        // Compare depths for shadows
        vec2 shadowMapUv = (segment.uvTransform * vec4(posInShadowMap.xy, 0.0, 1.0)).xy;
        vec2 offset = sampleRot * fibShadowSamples[i];
        shadowMapUv += fibRadius * shadowTexelSize * offset;
        float mapDepth = texture(u_shadow_map, shadowMapUv).x;

        float actualDepth = posInShadowMap.z * 0.5 + 0.5;
        float shadowFactor = (mapDepth < actualDepth + bias) ? 0.0 : 1.0;

        // TODO: Implement a proper UV-based way of detecting if we sample outside the shadow map segment

        shadowAcc += shadowFactor;
    }

    return shadowAcc / numFibShadowSamples;
}

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

    vec3 lightColor = rgbFromColor(directionalLight.color);
    vec3 directLight = lightColor * calculateShadowFactor(viewSpacePos, LdotN);

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
