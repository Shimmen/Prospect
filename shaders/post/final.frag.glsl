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

void main()
{
    vec3 hdrColor = texture(u_texture, v_uv).rgb;

    vec3 bloom = textureLod(u_bloom_texture, v_uv, 0.0).rgb;
    hdrColor = mix(hdrColor, bloom, u_bloom_amount);

    // Auto adjusting exposure
    //
    // TODO: implement proper camera model. See:
    //   https://knarkowicz.wordpress.com/2016/01/09/automatic-exposure/
    //   https://placeholderart.wordpress.com/2014/12/15/implementing-a-physically-based-camera-automatic-exposure/
    {
        // Sample the middle of the single pixel at the highest LoD. Since we perform a geometric
        // mean and take y=log(x) we undo that here by applying x'=exp(y) to get the average.
        float avgLuminance = exp(textureLod(u_avg_log_lum_texture, vec2(0.5), 99999.9).r);

        float adaptionRate = 0.004;
        float exposureKey = 0.1; // ?
        float lumMin = 0.1;
        float lumMax = 2.0;

        float newLum = clamp(avgLuminance, lumMin, lumMax);
        float oldLum = imageLoad(img_current_lum, ivec2(0)).r;

        float curLum = oldLum + (newLum - oldLum) * (1.0 - exp(-camera.delta_time * adaptionRate));
        imageStore(img_current_lum, ivec2(0), vec4(curLum));

        float autoExposure = exposureKey / curLum;
        hdrColor *= autoExposure;
    }

    // Manual exposure control/adjust
    hdrColor *= u_exposure;

    hdrColor *= naturalVignetting(u_vignette_falloff);

    vec3 ldrColor = tonemap(hdrColor);
    vec3 gammaCorrectLdr = gammaAdust(ldrColor, u_gamma);

    o_color = vec4(gammaCorrectLdr, 1.0);
}
