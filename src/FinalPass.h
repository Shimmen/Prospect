#pragma once

#include <glad/glad.h>

#include "ShaderDependant.h"
#include "LightBuffer.h"
#include "BloomPass.h"

class FinalPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer, BloomPass& bloomPass);
	void ProgramLoaded(GLuint program) override;

private:

	GLuint logLumTexture{ 0 };
	GLuint currentLumTexture{ 0 };

	GLuint *logLumProgram{ 0 };
	GLuint *finalProgram{ 0 };

};
