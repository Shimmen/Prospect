#pragma once

#include <vector>

#include <glad/glad.h>

#include "Model.h"
#include "GBuffer.h"
#include "FpsCamera.h"

class GeometryPass
{
public:

	bool performDepthPrepass = true;

	void Draw(const GBuffer& gBuffer, const std::vector<Model>& opaqueGeometry, FpsCamera& camera);

private:

	GLuint *depthOnlyProgram = nullptr;

};
