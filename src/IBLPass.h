#pragma once

#include <glad/glad.h>

#include "Scene.h"
#include "GBuffer.h"
#include "SSAOPass.h"
#include "LightBuffer.h"

struct DirectionalLight;

class IBLPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, const SSAOPass& ssaoPass, Scene& scene);
	void ProgramLoaded(GLuint program) override;

private:

	void CreateBrdfIntegrationMap();
	void FilterProbe(Probe& probe);

	GLuint iblProgram{ 0 };

	GLuint brdfIntegrationMap{ 0 };
	GLuint sphereSampleBuffer{ 0 };

};
