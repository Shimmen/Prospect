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

	float kernelRadius = 0.62f;
	float intensity = 7.0f;
	bool applyBlur = true;

	// (recompile to change this.. easier this way)
	bool randomKernelSamples = true;

private:

	void GenerateAndUpdateKernel() const;

	GLuint *ssaoProgram{ 0 };
	GLuint *ssaoBlurProgram{ 0 };

	GLuint ssaoDataBuffer{ 0 };
	SSAOData ssaoData{};

};
