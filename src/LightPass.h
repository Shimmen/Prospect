#pragma once

#include <glad/glad.h>

#include "Model.h"
#include "GBuffer.h"
#include "FpsCamera.h"
#include "ShadowMap.h"
#include "LightBuffer.h"

struct DirectionalLight;

class LightPass
{
public:

	void Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, const ShadowMap& shadowMap, FpsCamera& camera, DirectionalLight& dirLight);

private:

	GLuint emptyVertexArray = 0;

	GLuint *directionalLightProgram{ 0 };
	GLuint directionalLightUniformBuffer{ 0 };

};