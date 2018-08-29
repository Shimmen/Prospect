#ifndef SHADER_TYPES_H
#define SHADER_TYPES_H

// color.rgb is the color [0, 1] and color.a is [0, 1] but maps to a [0, 1000] exponential scale y=2^(10x)
#define Color vec4

struct ShadowMapSegment
{
	float zNear, zFar;
	float unused1, unused2;

	int minX, maxX;
	int minY, maxY;

	mat4 lightViewProjection;
};

struct DirectionalLight
{
	vec4 worldDirection;
    vec4 viewDirecion;

    Color color;

	// x = index, yzw = unused
	ivec4 shadowMapSegmentIndex;
};

#endif // SHADER_TYPES_H
