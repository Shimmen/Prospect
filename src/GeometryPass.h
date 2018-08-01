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

	GLenum framebuffer;

	GLuint lastBoundAlbedo = 0;
	GLuint lastBoundNormal = 0;
	GLuint lastBoundDepth = 0;

	GLuint *depthOnlyProgram = nullptr;

};
