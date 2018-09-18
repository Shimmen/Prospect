#pragma once

#include <glad/glad.h>

#include "Model.h"
#include "Scene.h"
#include "GBuffer.h"
#include "FpsCamera.h"
#include "ShadowMap.h"
#include "LightBuffer.h"
#include "ShaderDependant.h"

struct DirectionalLight;

class LightPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, const ShadowMap& shadowMap, Scene& scene);
	void ProgramLoaded(GLuint program) override;

private:

	GLuint emptyVertexArray{ 0 };

	GLuint directionalLightProgram{ 0 };
	GLuint directionalLightUniformBuffer{ 0 };

};
