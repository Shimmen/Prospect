#ifndef COMMON_GLSL
#define COMMON_GLSL

#include <shader_types.h>
#include <etc/octahedral.glsl>

#define PI     (3.14159265358979323846)
#define TWO_PI (2.0 * PI)

// color.rgb is the color [0, 1] and color.a is [0, 1] but maps to a [0, 63] exponential scale y=2^(6x)-1
// see shader_types.h for definitions
vec3 rgbFromColor(Color color)
{
    float x = color.a;
    float y = pow(2.0, 5.0 * x) - 1.0;
    return vec3(color.rgb * y);
}

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 unpackNormalMapNormal(vec3 normalMapNormal)
{
    vec3 N = normalize(normalMapNormal * vec3(2.0) - vec3(1.0));
    N.y *= -1.0; // (flip normal y to get correct up-axis)
    return N;
}

float lengthSquared(vec2 v)
{
    return dot(v, v);
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

void reortogonalize(in vec3 v0, inout vec3 v1)
{
	// Perform Gram-Schmidt's re-ortogonalization process to make v1 orthagonal to v0
	v1 = normalize(v1 - dot(v1, v0) * v0);
}

mat3 createTbnMatrix(vec3 tangent, vec3 bitangent, vec3 normal)
{
    reortogonalize(normal, tangent);
    reortogonalize(tangent, bitangent);
    reortogonalize(bitangent, normal);

    tangent = normalize(tangent);
    bitangent = normalize(bitangent);
    normal = normalize(normal);

    return mat3(tangent, bitangent, normal);
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

vec3 sampleShIrradiance(vec3 N, sampler2D shMap)
{
    // SH basis
    float Y00     = 0.282095;
    float Y11     = 0.488603 * N.x;
    float Y10     = 0.488603 * N.z;
    float Y1_1    = 0.488603 * N.y;
    float Y21     = 1.092548 * N.x * N.z;
    float Y2_1    = 1.092548 * N.y * N.z;
    float Y2_2    = 1.092548 * N.y * N.x;
    float Y20     = 0.946176 * N.z * N.z - 0.315392;
    float Y22     = 0.546274 * (N.x * N.x - N.y * N.y);

    // SH coefficients as RGB
    vec3 L00  = texelFetch(shMap, ivec2(0,0), 0).rgb;
    vec3 L11  = texelFetch(shMap, ivec2(1,0), 0).rgb;
    vec3 L10  = texelFetch(shMap, ivec2(2,0), 0).rgb;
    vec3 L1_1 = texelFetch(shMap, ivec2(0,1), 0).rgb;
    vec3 L21  = texelFetch(shMap, ivec2(1,1), 0).rgb;
    vec3 L2_1 = texelFetch(shMap, ivec2(2,1), 0).rgb;
    vec3 L2_2 = texelFetch(shMap, ivec2(0,2), 0).rgb;
    vec3 L20  = texelFetch(shMap, ivec2(1,2), 0).rgb;
    vec3 L22  = texelFetch(shMap, ivec2(2,2), 0).rgb;

    // Used for extracting irradiance from the SH, see paper:
    // https://graphics.stanford.edu/papers/envmap/envmap.pdf
    float A0 = PI;
    float A1 = 2.0 / 3.0 * PI;
    float A2 = 1.0 / 4.0 * PI;

    return A0*Y00*L00
        + A1*Y1_1*L1_1 + A1*Y10*L10 + A1*Y11*L11
        + A2*Y2_2*L2_2 + A2*Y2_1*L2_1 + A2*Y20*L20 + A2*Y21*L21 + A2*Y22*L22;
}

// Source: http://lolengine.net/blog/2013/07/27/rgb-to-hsv-in-glsl
vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

#endif // COMMON_GLSL
