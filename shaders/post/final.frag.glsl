#version 460

#include <common.glsl>
#include <shader_locations.h>
#include <camera_uniforms.h>

in vec2 v_uv;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera;
};

PredefinedUniform(sampler2D, u_texture);

uniform float u_exposure;
uniform float u_vignette_falloff;
uniform float u_gamma;

uniform sampler2D u_bloom_texture;
uniform float u_bloom_amount;

uniform sampler2D u_avg_log_lum_texture;
layout(binding = 0, r32f) restrict uniform image2D img_current_lum;

PredefinedOutput(vec4, o_color);

//

const float whiteValue = 11.2;
vec3 uncharted2Tonemap(vec3 x)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 tonemap(vec3 hdrColor)
{
    float exposureBias = 1.0;
    vec3 curr = uncharted2Tonemap(exposureBias * hdrColor);

    vec3 whiteScale = vec3(1.0) / uncharted2Tonemap(vec3(whiteValue));
    vec3 color = curr * whiteScale;

    return color;
}

vec3 gammaAdust(vec3 color, float gamma)
{
    return pow(color, vec3(1.0 / gamma));
}

float naturalVignetting(float falloff)
{
    // From: https://github.com/keijiro/KinoVignette

    mat4 mat = camera.projection_from_view;
    float aspectRatio = mat[1][1] / mat[0][0];

    vec2 coord = (v_uv - vec2(0.5)) * vec2(aspectRatio, 1.0) * 2.0;
    float rf = sqrt(dot(coord, coord)) * falloff;
    float rf2_1 = rf * rf + 1.0;
    float e = 1.0 / (rf2_1 * rf2_1);

    return e;
}

//
// The following three functions are from the 'Moving Frostbite to PBR' course notes:
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
//

float computeEV100(float aperture, float shutterTime, float ISO)
{
    // EV number is defined as:
    //   2^EV_s = N^2 / t and EV_s = EV_100 + log2(S/100)
    // This gives
    //   EV_s = log2(N^2 / t)
    //   EV_100 + log2(S/100) = log2 (N^2 / t)
    //   EV_100 = log2(N^2 / t) - log2(S/100)
    //   EV_100 = log2(N^2 / t . 100 / S)
    return log2(square(aperture) / shutterTime * 100.0 / ISO);
}

float computeEV100FromAvgLuminance(float avgLuminance)
{
    // We later use the middle gray at 12.7% in order to have
    // a middle gray at 18% with a sqrt (2) room for specular highlights
    // But here we deal with the spot meter measuring the middle gray
    // which is fixed at 12.5 for matching standard camera
    // constructor settings (i.e. calibration constant K = 12.5)
    // Reference : http://en.wikipedia.org/wiki/Film_speed
    return log2(avgLuminance * 100.0 / 12.5);
}

float convertEV100ToExposure(float EV100)
{
    // Compute the maximum luminance possible with H_sbs sensitivity
    // maxLum = 78 / (  S * q   ) * N^2 / t
    //        = 78 / (  S * q   ) * 2^EV_100
    //        = 78 / (100 * 0.65) * 2^EV_100
    //        = 1.2 * 2^EV
    // Reference : http://en.wikipedia.org/wiki/Film_speed
    float maxLuminance = 1.2 * pow(2.0, EV100);
    return 1.0 / maxLuminance;
}

void main()
{
    vec3 hdrColor = texture(u_texture, v_uv).rgb;

    vec3 bloom = textureLod(u_bloom_texture, v_uv, 0.0).rgb;
    hdrColor = mix(hdrColor, bloom, u_bloom_amount);

    // Auto adjusting exposure
    if (false)
    {
        // Sample the middle of the single pixel at the highest LoD. Since we perform a geometric
        // mean and take y=log(x) we undo that here by applying x'=exp(y) to get the average.
        float avgLuminance = exp(textureLod(u_avg_log_lum_texture, vec2(0.5), 99999.9).r);

        float ev100 = computeEV100FromAvgLuminance(avgLuminance);

        float adaptionRate = 0.004;
        float exposureKey = 0.1; // ?
        float lumMin = -10.0;
        float lumMax = +10.0;

        float newEV = clamp(avgLuminance, lumMin, lumMax);
        float oldEV = imageLoad(img_current_lum, ivec2(0)).r;

        float curEV = oldEV + (newEV - oldEV) * (1.0 - exp(-camera.delta_time * adaptionRate));
        imageStore(img_current_lum, ivec2(0), vec4(curEV));

        // TODO: Apply EC (exposure compensation)

        float exposure = convertEV100ToExposure(curEV);
        hdrColor *= exposure;
    }

    // Manual exposure control
    if (true)
    {
        float ev100 = computeEV100(camera.aperture, camera.shutter_speed, camera.iso);
        // TODO: Apply EC (exposure compensation)
        float exposure = convertEV100ToExposure(ev100);

        hdrColor *= exposure;
        hdrColor *= 18000.0; // Why is this needed?!
    }

    // Manual exposure control/adjust
    //hdrColor *= u_exposure;

    hdrColor *= naturalVignetting(u_vignette_falloff);

    // TODO: Choose some nice tonemapping function!
    vec3 ldrColor = tonemap(hdrColor);
    //vec3 ldrColor = hdrColor / (1.0 + luminance(hdrColor));

    vec3 gammaCorrectLdr = gammaAdust(ldrColor, u_gamma);

    o_color = vec4(gammaCorrectLdr, 1.0);
}
