#version 460

#include <common.glsl>
#include <shader_locations.h>
#include <shader_constants.h>
#include <camera_uniforms.h>
#include <camera_model.glsl>
#include <etc/aces.glsl>

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

uniform int u_tonemap_operator_selector;

PredefinedOutput(vec4, o_color);

void main()
{
    vec3 hdrColor = texture(u_texture, v_uv).rgb;

    vec3 bloom = textureLod(u_bloom_texture, v_uv, 0.0).rgb;
    hdrColor = mix(hdrColor, bloom, u_bloom_amount);

    float aspectRatio = camera.projection_from_view[1][1] / camera.projection_from_view[0][0];
    hdrColor *= naturalVignetting(u_vignette_falloff, aspectRatio, v_uv);

    vec3 ldrColor;
    switch (u_tonemap_operator_selector)
    {
        case TONEMAP_OP_ACES:
            ldrColor = ACES_tonemap(hdrColor);
            break;

        case TONEMAP_OP_REINHARD:
            ldrColor = hdrColor / (vec3(1.0) + hdrColor);
            break;

        case TONEMAP_OP_UNCHARTED_2:
            ldrColor = uncharted2Tonemap(hdrColor);
            break;

        case TONEMAP_OP_CLAMP:
            ldrColor = hdrColor;
            break;
    }

    vec3 gammaCorrectLdr = gammaAdust(ldrColor, u_gamma);

    o_color = vec4(gammaCorrectLdr, 1.0);
}
