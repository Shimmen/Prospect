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

// TODO: Use this for the automatic exposure!
//uniform sampler2D u_avg_log_lum_texture;
//layout(binding = 0, r32f) restrict uniform image2D img_current_lum;

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 imagePx = imageSize(img_light_buffer);

    if (pixelCoord.x < imagePx.x && pixelCoord.y < imagePx.y)
    {
        vec3 hdrColor = imageLoad(img_light_buffer, pixelCoord).rgb;

        // Manual exposure control
        {
            float ev100 = computeEV100(camera.aperture, camera.shutter_speed, camera.iso);
            ev100 -= camera.exposure_compensation;

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
