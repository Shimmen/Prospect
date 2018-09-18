#pragma once

#include <glad/glad.h>

#include "Scene.h"
#include "GBuffer.h"
#include "LightBuffer.h"

struct DirectionalLight;

class IBLPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, Scene& scene);
	void ProgramLoaded(GLuint program) override;

private:

	void CreateBrdfIntegrationMap();

	GLuint emptyVertexArray{ 0 };
	GLuint iblProgram{ 0 };

	GLuint brdfIntegrationMap{ 0 };

};
