#pragma once

#include <vector>

#include <glad/glad.h>

#include "ShadowMap.h"
#include "Model.h"

struct DirectionalLight;

class ShadowPass
{
public:

	void Draw(const ShadowMap& shadowMap, const std::vector<Model>& blockingGeomety, DirectionalLight& dirLight);

private:

	GLuint *shadowProgram;

	GLuint shadowMapSegmentUniformBuffer{ 0 };

};
