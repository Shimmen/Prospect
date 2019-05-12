#version 460

#include <common.glsl>
#include <camera_model.glsl>
#include <camera_uniforms.h>
#include <shader_locations.h>

layout(
    local_size_x = 32,
    local_size_y = 32
) in;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera;
};

layout(binding = 0, rgba32f) restrict uniform image2D img_light_buffer;

layout(binding = 1, r32f) restrict readonly uniform image2D img_avg_log_lum;
layout(binding = 2, r32f) restrict          uniform image2D img_history_lum;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imagePx = imageSize(img_light_buffer);

    if (pixelCoord.x < imagePx.x && pixelCoord.y < imagePx.y)
    {
        vec3 hdrColor = imageLoad(img_light_buffer, pixelCoord).rgb;

        if (camera.use_automatic_exposure)
        {
            // Read current average luminance
            float avgLogLuminance = imageLoad(img_avg_log_lum, ivec2(0)).r;
            float avgLuminance = exp(avgLogLuminance);

            // Read history of average luminances
            float histLuminance = imageLoad(img_history_lum, ivec2(0)).r;

            // Compute the actual luminance to use, from history & current, and write back to next history sample
            float realLuminance = histLuminance + (avgLuminance - histLuminance)
                                                * (1.0 - exp(-camera.delta_time * camera.adaption_rate));
            imageStore(img_history_lum, ivec2(0), vec4(realLuminance));

            float ev100 = computeEV100FromAvgLuminance(realLuminance);
            ev100 -= camera.exposure_compensation;

            float exposure = convertEV100ToExposure(ev100);
            hdrColor *= exposure;
        }
        else
        {
            float ev100 = computeEV100(camera.aperture, camera.shutter_speed, camera.iso);
            float exposure = convertEV100ToExposure(ev100);
            hdrColor *= exposure;

            //
            // Why is this needed?!
            //
            hdrColor *= 120000.0;
        }

        // TODO: Maybe we wanna put the luminance in alpha or something like that?
        imageStore(img_light_buffer, pixelCoord, vec4(hdrColor, 1.0));
    }
}
