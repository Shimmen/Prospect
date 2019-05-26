#version 460

#include <common.glsl>
#include <shader_locations.h>
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

PredefinedOutput(vec4, o_color);

void main()
{
    vec3 hdrColor = texture(u_texture, v_uv).rgb;

    vec3 bloom = textureLod(u_bloom_texture, v_uv, 0.0).rgb;
    hdrColor = mix(hdrColor, bloom, u_bloom_amount);

    float aspectRatio = camera.projection_from_view[1][1] / camera.projection_from_view[0][0];
    hdrColor *= naturalVignetting(u_vignette_falloff, aspectRatio, v_uv);

    // TODO: Add some type of selector in the GUI for this!

    // Clamp:
    //vec3 ldrColor = hdrColor;

    // Basic Reinhard
    //vec3 ldrColor = hdrColor / (vec3(1.0) + hdrColor);

    // Uncharted2:
    //vec3 ldrColor = uncharted2Tonemap(hdrColor);

    // ACES:
    vec3 ldrColor = ACES_tonemap(hdrColor);



    vec3 gammaCorrectLdr = gammaAdust(ldrColor, u_gamma);

    o_color = vec4(gammaCorrectLdr, 1.0);
}
