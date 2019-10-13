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
        /*
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
        */


        // Alternative, slower blur:

        int k = 2;
        float totalWeight = 0.0;
        float o = 0.0;
        for (int i = -k; i <= k; ++i)
        {
            for (int j = -k; j <= k; ++j)
            {
                float weight = float(k) + 2.5 - length(vec2(i, j));
                o += weight * imageLoad(img_occlusion, pixelCoord + ivec2(i, j)).r;
                totalWeight += weight;
            }
        }
        o /= totalWeight;
        imageStore(img_occlusion, pixelCoord, vec4(o));
    }
}
