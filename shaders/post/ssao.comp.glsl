#version 460

#include <common.glsl>
#include <brdf.glsl>

#include <shader_locations.h>
#include <shader_constants.h>
#include <camera_uniforms.h>
#include <shader_types.h>
#include <ssao_data.h>

layout(
    local_size_x = 32,
    local_size_y = 32
) in;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera;
};

PredefinedUniformBlock(SSAODataBlock)
{
    SSAOData ssao;
};

PredefinedUniform(sampler2D, u_g_buffer_normal);
PredefinedUniform(sampler2D, u_g_buffer_depth);

layout(binding = 0, r32f) restrict writeonly uniform image2D img_occlusion;

vec3 project(vec3 vsPos)
{
    vec4 projPos = camera.projection_from_view * vec4(vsPos, 1.0);
    return projPos.xyz / projPos.w;
}

vec3 unproject(vec3 projPos)
{
    vec4 vsPos = camera.view_from_projection * vec4(projPos, 1.0);
    return vsPos.xyz / vsPos.w;
}

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imagePx = imageSize(img_occlusion);

    if (pixelCoord.x < imagePx.x && pixelCoord.y < imagePx.y)
    {
        vec3 packedNormal = texelFetch(u_g_buffer_normal, pixelCoord, 0).xyz;
        vec3 N = unpackNormal(packedNormal);
        float depth = texelFetch(u_g_buffer_depth, pixelCoord, 0).r;

        vec2 uv = (vec2(pixelCoord) + vec2(0.5)) / imageSize(img_occlusion);

        // Get view space position of fragment
        vec3 origin = unproject(vec3(uv * vec2(2.0) - vec2(1.0), depth * 2.0 - 1.0));
        //origin += N * vec3(0.125); // some amount of normal offset that works

        // Set up tbn matrix for orienting the kernel. Since we are rotating the TBN-matrix
        // with blue noise we really don't want to introduce any noise here at all, so all
        // noise is fully blue. However, since there isn't one orthogonal vector for any normal
        // we have to compute it here and introduce some noise that way. Fortunately it's
        // somewhat spatially consistent, so it's not very noisy to begin with... so this works.
        vec3 ortogonal = vec3(-N.y, N.x, 0.0);
        vec3 tangent = normalize(ortogonal - N * dot(ortogonal, N));
        vec3 bitangent = cross(tangent, N);
        mat3 tbn = mat3(tangent, bitangent, N);

        //
        // TODO: Rotate the TBN-matrix!!!
        //

        float occlusion = 0.0;
        for (int i = 0; i < SSAO_KERNEL_SAMPLE_COUNT; ++i)
        {
            // Calculate sample view space position
            vec3 samplePos = tbn * ssao.kernel[i].xyz;
            samplePos = origin + (samplePos * ssao.kernel_radius);
            float vsSampleDepth = samplePos.z;

            // Get actual/reference depth at sample
            vec3 projSamplePos = project(samplePos);
            vec2 sampleUv = projSamplePos.xy * vec2(0.5) + vec2(0.5);
            float projReferenceDepth = texture(u_g_buffer_depth, sampleUv).r;
            vec3 vsRefPosition = unproject(vec3(projSamplePos.xy, projReferenceDepth * 2.0 - 1.0));
            float vsReferenceDepth = vsRefPosition.z;

            float rangeCheck = 1.0;//smoothstep(0.0, 1.0, ssao.kernel_radius / abs(origin.z - vsReferenceDepth));
            occlusion += (vsSampleDepth < vsReferenceDepth ? 1.0 : 0.0) * rangeCheck;
        }
        occlusion /= float(SSAO_KERNEL_SAMPLE_COUNT);

        // Intensify the ambient occlusion, i.e. make occluded parts darker
        occlusion = pow(occlusion, ssao.intensity);

        // Ignore non-rendered/background pixels!
        if (depth >= 1.0)
        {
            occlusion = 1.0;
        }

        imageStore(img_occlusion, pixelCoord, vec4(occlusion));
    }
}
