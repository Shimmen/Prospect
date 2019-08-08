#pragma once

#include <glad/glad.h>

#include "Scene.h"
#include "ShadowMap.h"

struct ShadowMapSegment;
struct DirectionalLight;

class ShadowPass
{
public:

	void Draw(const ShadowMap& shadowMap, Scene& scene);

private:

	ShadowMapSegment CreateShadowMapSegmentForDirectionalLight(const ShadowMap& shadowMap, const DirectionalLight& dirLight);

	GLuint *shadowProgram{ 0 };
	GLuint shadowMapSegmentUniformBuffer{ 0 };

};
