#ifndef COMMON_GLSL
#define COMMON_GLSL

float lengthSquared(vec3 v)
{
    return dot(v, v);
}

float saturate(float x)
{
    return clamp(x, 0.0, 1.0);
}

#endif // COMMON_GLSL
