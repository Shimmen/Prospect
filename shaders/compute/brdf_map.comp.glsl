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

    const uint NUM_SAMPLES = 1024;
    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        // ...
    }

    //
    // TODO: Calculate BRDF integration map for the specular BRDF defined in brdf.glsl!
    //

    vec2 uv = vec2(pixelCoord) / vec2(imageSize(u_brdf_map));
    vec3 color = ggxBrdfApproximation(vec3(1.0), uv.y, uv.x);

    imageStore(u_brdf_map, pixelCoord, vec4(color, 1.0));
}
