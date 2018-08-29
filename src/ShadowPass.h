#pragma once

#include <vector>

#include <glad/glad.h>

#include "Scene.h"
#include "Model.h"
#include "ShadowMap.h"

struct DirectionalLight;

class ShadowPass
{
public:

	void Draw(const ShadowMap& shadowMap, Scene& scene);

private:

	GLuint *shadowProgram;

	GLuint shadowMapSegmentUniformBuffer{ 0 };

};
