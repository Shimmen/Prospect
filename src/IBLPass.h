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

	bool directLight = true;
	bool indirectLight = true;

private:

	GLuint emptyVertexArray{ 0 };

	//GLuint *directIBLProgram{ 0 };
	GLuint *indirectIBLProgram{ 0 };

};
#pragma once
