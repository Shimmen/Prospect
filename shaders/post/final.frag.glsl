#version 460

#include <common.glsl>
#include <shader_locations.h>
#include <camera_uniforms.h>

in vec2 v_uv;

PredefinedUniformBlock(CameraUniformBlock)
{
    CameraUniforms camera_uniforms;
};

PredefinedUniform(sampler2D, u_texture);
uniform float u_exposure;

PredefinedOutput(vec4, o_color);

float A = 0.15;
float B = 0.50;
float C = 0.10;
float D = 0.20;
float E = 0.02;
float F = 0.30;
float W = 11.2;

vec3 uncharted2Tonemap(vec3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
    vec3 hdrColor = texture(u_texture, v_uv).rgb;
    hdrColor *= u_exposure;

    float exposureBias = 1.0;
    vec3 curr = uncharted2Tonemap(exposureBias * hdrColor);

    vec3 whiteScale = vec3(1.0) / uncharted2Tonemap(vec3(W));
    vec3 color = curr * whiteScale;

    vec3 ldrColor = pow(color, vec3(1.0 / 2.2));
    o_color = vec4(ldrColor, 1.0);
}