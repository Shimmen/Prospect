#version 460

#include <common.glsl>
#include <brdf.glsl>

#include <shader_locations.h>
#include <shader_constants.h>
#include <camera_uniforms.h>
#include <shader_types.h>

in vec2 v_uv;
in vec3 v_view_ray;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

PredefinedUniform(sampler2D, u_g_buffer_albedo);
PredefinedUniform(sampler2D, u_g_buffer_material);
PredefinedUniform(sampler2D, u_g_buffer_normal);
PredefinedUniform(sampler2D, u_g_buffer_depth);

uniform sampler2D u_radiance;
uniform sampler2D u_brdf_integration_map;
uniform sampler2D u_irradiance_sh;

PredefinedOutput(vec4, o_color);

float linearizeDepth(float nonLinearDepth)
{
    float projectionA = camera_uniforms.near_far.z;
    float projectionB = camera_uniforms.near_far.w;
    return projectionB / (nonLinearDepth - projectionA);
}

vec3 ggxBrdfApproximation(vec3 specularColor, float roughness, float NdotV)
{
    // From https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
    const vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    vec2 AB = vec2(-1.04, 1.04) * a004 + r.zw;
    return specularColor * AB.x + AB.y;
}

void main()
{
    float depth = texture(u_g_buffer_depth, v_uv).x;
    vec4 viewSpacePos = vec4(v_view_ray * linearizeDepth(depth), 1.0);

    vec3 packedNormal = texture(u_g_buffer_normal, v_uv).xyz;
    bool unlit = lengthSquared(packedNormal) < 0.0001;

    vec3 N = unpackNormal(packedNormal);
    vec3 V = -normalize(viewSpacePos.xyz);

    // This roughness parameter is supposed to be perceptually linear! Since we use a squared
    // roughness when prefiltering the radiance map this works itself out in this shader.
    vec4 material = texture(u_g_buffer_material, v_uv);
    float roughness = material.x;
    float metallic = material.y;

    vec3 baseColor = texture(u_g_buffer_albedo, v_uv).xyz;

    vec3 f0 = mix(vec3(DIELECTRIC_REFLECTANCE), baseColor, metallic);
    // (use square roughness here, since we call directly into the brdf-related code)
    vec3 F = F_SchlickRoughnessCompensating(saturate(dot(V, N)), f0, square(roughness));

    mat3 world_from_view_dir = mat3(camera_uniforms.world_from_view);

    vec3 specular;
    {
        vec3 R = world_from_view_dir * reflect(-V, N);
        R.y *= -1.0; // TODO: Fix image loading y-axis!

        float sampleLoD = roughness * float(IBL_RADIANCE_MIPMAP_LAYERS - 1);
        vec3 prefiltered = textureLod(u_radiance, sphericalUvFromDirection(R), sampleLoD).rgb;

        //vec2 AB = ggxBrdfApproximation(F, roughness, saturate(dot(N, V)));
        vec2 AB = texture(u_brdf_integration_map, vec2(saturate(dot(N, V)), roughness)).xy;

        specular = prefiltered * (f0 * AB.x + AB.y);
    }

    vec3 diffuse;
    {
        vec3 dir = world_from_view_dir * N;
        dir.y *= -1.0; // TODO: Fix image loading y-axis!

        // (in case there is a surface with a broken normal, "mask" it out,
        //  because if it's visible this below will produce NaNs wich will
        //  be blown up in size due to the bloom)
        dir = max(dir, 0.0);

        vec3 irradiance = max(sampleShIrradiance(dir, u_irradiance_sh), vec3(0.0));
        diffuse = baseColor * irradiance * PI; // Feels like there should be a *divide* by PI here, but this seems to be energy conserving...
    }

    vec3 kDiffuse = (vec3(1.0) - F) * vec3(1.0 - metallic);
    vec3 indirectLight = (kDiffuse * diffuse) + specular;

    vec3 color = (unlit) ? baseColor : indirectLight;
    o_color = vec4(color, 1.0);
}
