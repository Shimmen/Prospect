#ifndef OCTAHEDRAL_GLSL
#define OCTAHEDRAL_GLSL

// Efficient GPU implementation of the octahedral unit vector encoding from Cigolle, Donow, Evangelakos, Mara, McGuire, Meyer,
// A Survey of Efficient Representations for Independent Unit Vectors, Journal of Computer Graphics Techniques (JCGT), vol. 3, no. 2, 1-30, 2014
// Available online http://jcgt.org/published/0003/02/01/

float signNotZero(float f) { return(f >= 0.0) ? 1.0 : -1.0; }
vec2  signNotZero(vec2  v) { return vec2(signNotZero(v.x), signNotZero(v.y)); }

// Assumes that v is a unit vector. The result is an octahedral vector on the [-1, +1] square.
vec2 octahedralEncode(vec3 v)
{
    float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
    vec2 result = v.xy * (1.0 / l1norm);
    if (v.z < 0.0)
    {
        result = (1.0 - abs(result.yx)) * signNotZero(result.xy);
    }
    return result;
}


// Returns a unit vector. Argument o is an octahedral vector packed via octahedralEncode, on the [-1, +1] square
vec3 octahedralDecode(vec2 o)
{
    vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
    if (v.z < 0.0)
    {
        v.xy = (1.0 - abs(v.yx)) * signNotZero(v.xy);
    }
    return normalize(v);
}

#endif // OCTAHEDRAL_GLSL
