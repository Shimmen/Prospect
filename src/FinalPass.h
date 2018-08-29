#pragma once

#include <glad/glad.h>

#include "LightBuffer.h"

class FinalPass
{
public:

	void Draw(const LightBuffer& lightBuffer);

private:

	GLuint emptyVertexArray = 0;
	GLuint *finalProgram{ 0 };

};
