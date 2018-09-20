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
    hdrColor *= u_exposure;

    hdrColor *= naturalVignetting(u_vignette_falloff);

    vec3 ldrColor = tonemap(hdrColor);
    vec3 gammaCorrectLdr = gammaAdust(ldrColor, u_gamma);

    o_color = vec4(gammaCorrectLdr, 1.0);
}
