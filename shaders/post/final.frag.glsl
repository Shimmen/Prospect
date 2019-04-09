#version 460

#include <common.glsl>
#include <shader_locations.h>
#include <camera_uniforms.h>
#include <camera_model.glsl>

in vec2 v_uv;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera;
};

PredefinedUniform(sampler2D, u_texture);

uniform sampler2D u_bloom_texture;
uniform float u_bloom_amount;

uniform float u_vignette_falloff;
uniform float u_gamma;

PredefinedOutput(vec4, o_color);

void main()
{
    vec3 hdrColor = texture(u_texture, v_uv).rgb;

    vec3 bloom = textureLod(u_bloom_texture, v_uv, 0.0).rgb;
    hdrColor = mix(hdrColor, bloom, u_bloom_amount);

    float aspectRatio = camera.projection_from_view[1][1] / camera.projection_from_view[0][0];
    hdrColor *= naturalVignetting(u_vignette_falloff, aspectRatio, v_uv);

    // TODO: Choose some nice tonemapping function!
    vec3 ldrColor = tonemap(hdrColor);
    //vec3 ldrColor = hdrColor / (1.0 + luminance(hdrColor));

    vec3 gammaCorrectLdr = gammaAdust(ldrColor, u_gamma);

    o_color = vec4(gammaCorrectLdr, 1.0);
}
