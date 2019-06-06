#pragma once

#include <glad/glad.h>

#include "Scene.h"
#include "GBuffer.h"
#include "LightBuffer.h"

class SkyPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, Scene& scene);
	void ProgramLoaded(GLuint program) override;

private:

	GLuint framebuffer{ 0 };
	GLuint skyProgram{ 0 };

};
