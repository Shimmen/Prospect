#version 460

#include <common.glsl>
#include <shader_locations.h>
#include <camera_uniforms.h>

#include <shader_types.h>

in vec2 v_uv;

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

PredefinedOutput(vec4, o_color);

void main()
{
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
        /*
        int segmentIdx = directionalLight.shadowMapSegmentIndex.x;
        ShadowMapSegment segment = shadowMapSegments[segmentIdx];
        */
    }

    // diffuse term
    float LdotN = (unlit) ? 1.0 : dot(L, N);
    if (LdotN > 0.0)
    {
        color += albedo * lightColor * LdotN * shadowFactor;
    }

    o_color = vec4(color, 1.0);
}
