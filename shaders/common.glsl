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

float square(float x)
{
    return x * x;
}

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

vec2 sphericalUvFromDirection(vec3 direction)
{
    float phi = atan(direction.z, direction.x);
    float theta = acos(clamp(direction.y, -1.0, +1.0));

    if (phi < 0.0) phi += TWO_PI;
	return vec2(phi / TWO_PI, theta / PI);
}

vec3 directionFromSphericalUv(vec2 uv)
{
    float phi = uv.x * TWO_PI;
    float theta = uv.y * PI;

    float sinTheta = sin(theta);

    return vec3(
        cos(phi) * sinTheta,
        cos(theta),
        sin(phi) * sinTheta
    );
}

vec2 hammersley(uint i, uint n)
{
    uint bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float xi1 = float(bits) * 2.3283064365386963e-10; // / 0x100000000

    float xi0 = float(i) / float(n);
    return vec2(xi0, xi1);
}

#endif // COMMON_GLSL
