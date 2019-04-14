#pragma once

#include <glad/glad.h>

#include "GBuffer.h"
#include "ShaderDependant.h"

#include <glm/glm.hpp>
using namespace glm;
#include "ssao_data.h"

class SSAOPass: public ShaderDepandant
{
public:

	void Draw(const GBuffer& gBuffer);
	void ProgramLoaded(GLuint program) override;

	GLuint occlusionTexture{ 0 };

	float kernelRadius = 1.5f;
	float intensity = 6.0f;

private:

	void GenerateAndUpdateKernel() const;

	GLuint kernelNoiseTexture{ 0 };

	GLuint *ssaoProgram{ 0 };

	GLuint ssaoDataBuffer{ 0 };
	SSAOData ssaoData{};

};
