#pragma once

#include <glad/glad.h>

#include "ShaderDependant.h"
#include "LightBuffer.h"
#include "BloomPass.h"
#include "Scene.h"

class FinalPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer, Scene& scene);
	void ProgramLoaded(GLuint program) override;

private:

	BloomPass bloomPass;

	GLuint logLumTexture{ 0 };
	GLuint currentLumTexture{ 0 };

	GLuint *exposureProgram{ 0 };
	GLuint *logLumProgram{ 0 };
	GLuint *finalProgram{ 0 };

};
