#ifndef COMMON_GLSL
#define COMMON_GLSL

#include <shader_types.h>

#define PI     (3.14159265358979323846)
#define TWO_PI (2.0 * PI)

// color.rgb is the color [0, 1] and color.a is [0, 1] but maps to a [0, 1000] exponential scale y=2^(10x)
// see shader_types.h for definitions
vec3 rgbFromColor(Color color)
{
    float x = color.a;
    float y = pow(2.0, 10.0*x);
    return vec3(color.rgb * y);
}

vec3 packNormal(vec3 normal)
{
    return normalize(normal) * vec3(0.5) + vec3(0.5);
}

vec3 unpackNormal(vec3 packedNormal)
{
    return normalize(packedNormal * vec3(2.0) - vec3(1.0));
}

float lengthSquared(vec3 v)
{
    return dot(v, v);
}

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

vec2 sphericalFromDirection(vec3 direction)
{
    float theta = acos(clamp(direction.y, -1.0, 1.0));
    float phi = atan(direction.z, direction.x);
    if (phi < 0.0) phi += TWO_PI;
	return vec2(phi / TWO_PI, theta / PI);
}

#endif // COMMON_GLSL
