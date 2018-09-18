#pragma once

#include <glad/glad.h>

#include "Scene.h"
#include "GBuffer.h"
#include "LightBuffer.h"

struct DirectionalLight;

class IBLPass
{
public:

	void Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, Scene& scene);

	bool indirectLight = true;

private:

	GLuint emptyVertexArray{ 0 };
	GLuint *iblProgram{ 0 };

};
#pragma once
