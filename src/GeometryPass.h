#pragma once

#include <vector>

#include <glad/glad.h>

#include "Scene.h"
#include "Model.h"
#include "GBuffer.h"
#include "FpsCamera.h"
#include "ShaderDependant.h"

class GeometryPass : ShaderDepandant
{
public:

	bool performDepthPrepass = false;
	bool wireframeRendering = false;

	void Draw(const GBuffer& gBuffer, Scene& scene);
	void ProgramLoaded(GLuint program) override;

private:

	GLuint depthOnlyProgram{ 0 };
	GLint modelMatrixLocation;

};
