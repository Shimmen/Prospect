#pragma once

#include <glad/glad.h>

#include "Scene.h"
#include "LightBuffer.h"

class SkyPass
{
public:

	void Draw(const LightBuffer& lightBuffer, Scene& scene);

private:

	GLuint emptyVertexArray{ 0 };
	GLuint *skyProgram{ 0 };

};

