#pragma once

#include <vector>

#include <glad/glad.h>

#include "Model.h"
#include "GBuffer.h"
#include "FpsCamera.h"
#include "LightBuffer.h"

class LightPass
{
public:

	void Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, FpsCamera& camera);

private:

	GLuint emptyVertexArray = 0;

};
#pragma once
