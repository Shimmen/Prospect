#pragma once

#include <glad/glad.h>

#include "ShaderDependant.h"
#include "TemporalAAPass.h"
#include "LightBuffer.h"
#include "BloomPass.h"
#include "GBuffer.h"
#include "Scene.h"

class FinalPass : ShaderDepandant
{
public:

	void Draw(const GBuffer& gBuffer, const LightBuffer& lightBuffer, Scene& scene);
	void ProgramLoaded(GLuint program) override;

private:

	BloomPass bloomPass;
	TemporalAAPass taaPass;

	GLuint logLumTexture{ 0 };
	GLuint currentLumTexture{ 0 };

	GLuint *exposureProgram{ 0 };
	GLuint *logLumProgram{ 0 };
	GLuint *finalProgram{ 0 };

};
