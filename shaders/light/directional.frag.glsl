#version 460

#include <common.glsl>
#include <shader_locations.h>
#include <camera_uniforms.h>

#include <shader_types.h>

in vec2 v_uv;
in vec3 v_view_ray;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

PredefinedUniformBlock(ShadowMapSegmentBlock)
{
    ShadowMapSegment shadowMapSegments[32];
    int shadowMapSegmentCount;
};

PredefinedUniformBlock(DirectionalLightBlock)
{
    DirectionalLight directionalLight;
};

PredefinedUniform(sampler2D, u_g_buffer_albedo);
PredefinedUniform(sampler2D, u_g_buffer_normal);
PredefinedUniform(sampler2D, u_g_buffer_depth);

PredefinedUniform(sampler2D, u_shadow_map);

PredefinedOutput(vec4, o_color);

void main()
{
    // TODO: Pass these in through the camera uniforms!
    // Calculate the projection constants
    float zNear = camera_uniforms.near_far.x;
    float zFar  = camera_uniforms.near_far.y;
    float projectionA = zFar / (zFar - zNear);
    float projectionB = (-zFar * zNear) / (zFar - zNear);

    // Sample the depth and convert to linear view space Z
    float depth = texture(u_g_buffer_depth, v_uv).x;
    float linearDepth = projectionB / (depth - projectionA);
    vec4 viewSpacePos = vec4(v_view_ray * linearDepth, 1.0);

    //

    vec3 packedNormal = texture(u_g_buffer_normal, v_uv).xyz;
    bool unlit = lengthSquared(packedNormal) < 0.0001;
    vec3 N = normalize(packedNormal * vec3(2.0) - vec3(1.0));

    vec3 worldLightDirection = vec3(-0.2, -1.0, -0.2);
    vec3 viewLightDirection = mat3(camera_uniforms.view_from_world) * worldLightDirection;
    vec3 L = -normalize(viewLightDirection);

    vec3 albedo = texture(u_g_buffer_albedo, v_uv).xyz;
    vec3 lightColor = rgbFromColor(directionalLight.color);

    vec3 color = vec3(0.0);

    // ambient term
    const float ambient = 0.02;
    color += albedo * lightColor * ambient * float(!unlit);

    // shadow factor
    float shadowFactor = 1.0;
    {
        int segmentIdx = directionalLight.shadowMapSegmentIndex.x;
        ShadowMapSegment segment = shadowMapSegments[segmentIdx];

        mat4 lightProjectionFromView = segment.lightViewProjection * camera_uniforms.world_from_view;
        vec4 posInShadowMap = lightProjectionFromView * viewSpacePos;
        posInShadowMap.xyz /= posInShadowMap.w;

        float actualDepth = posInShadowMap.z * 0.5 + 0.5;
        vec2 segmentUv = posInShadowMap.xy * vec2(0.5) + vec2(0.5);

        ivec2 shadowMapSize = textureSize(u_shadow_map, 0);
        ivec2 segmentMin = ivec2(segment.minX, segment.minY);
        ivec2 segmentMax = ivec2(segment.maxX, segment.maxY);
        vec2 segmentMinUv = vec2(segmentMin) / vec2(shadowMapSize);
        vec2 segmentMaxUv = vec2(segmentMax) / vec2(shadowMapSize);
        vec2 segmentSizeUv = segmentMaxUv - segmentMinUv;

        vec2 shadowMapUv = vec2(segmentMinUv + segmentUv * segmentSizeUv);
        float mapDepth = texture(u_shadow_map, shadowMapUv).x;

        shadowFactor = (mapDepth < actualDepth) ? 0.0 : 1.0;
    }

    // diffuse term
    float LdotN = dot(L, N);
    if (LdotN > 0.0)
    {
        color += albedo * lightColor * LdotN * shadowFactor;
    }

    color = (unlit) ? albedo : color;
    o_color = vec4(color, 1.0);
}
