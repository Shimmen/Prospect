#version 460

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

PredefinedUniform(sampler2D, u_g_buffer_albedo);
PredefinedUniform(sampler2D, u_g_buffer_normal);
PredefinedUniform(sampler2D, u_g_buffer_depth);

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

    vec3 packedNormal = texture(u_g_buffer_normal, v_uv).xyz;
    bool unlit = lengthSquared(packedNormal) < 0.0001;

    vec3 N = unpackNormal(packedNormal);
    vec3 V = -normalize(viewSpacePos.xyz);

    vec3 albedo = texture(u_g_buffer_albedo, v_uv).xyz;
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    vec3 color = vec3(0.0);

    // ambient term (TODO: Remove!)
    const float ambient = 0.5;
    color += albedo * lightColor * ambient * float(!unlit);

    color = (unlit) ? albedo : color;
    o_color = vec4(color, 1.0);
}
