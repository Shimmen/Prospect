#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <vector>

struct ShadowMap
{
	int size;

	GLuint framebuffer;

	// DEPTH_COMPONENT_32F
	GLuint texture = 0;

	////////////////////////////

	void RecreateGpuResources(int size);

};
