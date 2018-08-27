#ifndef COMMON_GLSL
#define COMMON_GLSL

#include <shader_types.h>

// color.rgb is the color [0, 1] and color.a is [0, 1] but maps to a [0, 1000] exponential scale y=2^(10x)
// see shader_types.h for definitions
vec3 rgbFromColor(Color color)
{
    float x = color.a;
    float y = pow(2.0, 10.0*x);
    return vec3(color.rgb * y);
}

float lengthSquared(vec3 v)
{
    return dot(v, v);
}

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

#endif // COMMON_GLSL
