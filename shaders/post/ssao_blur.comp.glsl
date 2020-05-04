#version 460

#include <common.glsl>

layout(
    local_size_x = 32,
    local_size_y = 32
) in;

layout(binding = 0, r16f) restrict uniform image2D img_occlusion;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imagePx = imageSize(img_occlusion);

    if (pixelCoord.x < imagePx.x && pixelCoord.y < imagePx.y)
    {
#if 0

        float occlusionCenter = imageLoad(img_occlusion, pixelCoord).r;

        vec4 occlusionPlus = vec4(
            imageLoad(img_occlusion, pixelCoord + ivec2(+1,  0)).r,
            imageLoad(img_occlusion, pixelCoord + ivec2(-1,  0)).r,
            imageLoad(img_occlusion, pixelCoord + ivec2( 0, +1)).r,
            imageLoad(img_occlusion, pixelCoord + ivec2( 0, -1)).r
        );

        vec4 occlusionCross = vec4(
            imageLoad(img_occlusion, pixelCoord + ivec2(+1, +1)).r,
            imageLoad(img_occlusion, pixelCoord + ivec2(+1, -1)).r,
            imageLoad(img_occlusion, pixelCoord + ivec2(-1, +1)).r,
            imageLoad(img_occlusion, pixelCoord + ivec2(-1, -1)).r
        );

        float occlusion = 0.55 * occlusionCenter
            + 0.30 * dot(vec4(0.25), occlusionPlus)
            + 0.15 * dot(vec4(0.25), occlusionCross);

        imageStore(img_occlusion, pixelCoord, vec4(occlusion));

#else
        // Alternative and better but slower blur:
        const int k = 2;

        float o = 0.0;
        float totalWeight = 0.0;

        for (int i = -k; i <= k; ++i)
        {
            for (int j = -k; j <= k; ++j)
            {
                // (make the weight slightly bell shaped)
                float weight = 1.0 + float(k*k) - length(vec2(i, j));
                totalWeight += weight;

                o += weight * imageLoad(img_occlusion, pixelCoord + ivec2(i, j)).r;
            }
        }
        o /= totalWeight;
        imageStore(img_occlusion, pixelCoord, vec4(o));
#endif
    }
}
