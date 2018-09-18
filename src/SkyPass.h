#pragma once

#include <glad/glad.h>

#include "Scene.h"
#include "LightBuffer.h"

class SkyPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer, Scene& scene);
	void ProgramLoaded(GLuint program) override;

private:

	GLuint emptyVertexArray{ 0 };
	GLuint skyProgram{ 0 };

};

