#pragma once

#include <vector>

#include <glad/glad.h>

#include "Scene.h"
#include "Model.h"
#include "GBuffer.h"
#include "FpsCamera.h"

class GeometryPass
{
public:

	bool performDepthPrepass = true;

	void Draw(const GBuffer& gBuffer, Scene& scene);

private:

	GLuint *depthOnlyProgram = nullptr;

};
